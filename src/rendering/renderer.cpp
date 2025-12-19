#include "rendering/renderer.hpp"

#include "rendering/components/mesh_renderer.hpp"
#include "rendering/components/transform.hpp"
#include "vulkan/material.hpp"

namespace rendering {
Renderer::Renderer(Window& window, bool enableValidationLayers)
    : window_{window}, enableValidationLayers_{enableValidationLayers} {
  if (isInitialized_) {
    LOG_WARNING("Renderer already initialized.");
    return;
  }

  LOG_DEBUG("Initializing Renderer...");

  window.AddOnResize(
      [&](int width, int height) { this->Resize(width, height); });

  context_ =
      std::make_unique<vulkan::Context>(window_, enableValidationLayers_);
  commandSystem_ = std::make_unique<vulkan::CommandSystem>(*context_);
  swapchain_ = std::make_unique<vulkan::Swapchain>(
      *context_, static_cast<uint32_t>(window_.GetWidth()),
      static_cast<uint32_t>(window_.GetHeight()));
  frameManager_ =
      std::make_unique<vulkan::FrameManager>(*context_, *commandSystem_);
  globalShaderData_ = std::make_unique<vulkan::GlobalShaderData>(*context_);
  materialPalette_ = std::make_unique<vulkan::MaterialPalette>(*context_);

  vulkan::Material defaultMat{.diffuseIndex = 0, .normalIndex = 1};
  materialPalette_->AddMaterial(defaultMat);

  assetLoader_ = std::make_unique<vulkan::AssetLoader>(*context_);
  testMesh_ = std::make_shared<vulkan::Mesh>(
      assetLoader_->LoadMesh("assets/models/Cube.gltf"));

  basicPipeline_ = std::make_unique<BasicPipeline>(
      *context_, *swapchain_, *globalShaderData_, *materialPalette_);

  renderingSystem_ = std::make_unique<RenderingSystem>();

  // Create a test entity
  auto entity = registry_.create();
  registry_.emplace<Transform>(entity, glm::vec3{0.0F, 0.0F, 0.0F}, glm::quat{},
                               glm::vec3{1.0F});
  registry_.emplace<MeshRenderer>(entity, testMesh_, 0);

  isInitialized_ = true;
  LOG_DEBUG("Renderer initialized successfully.");
}

Renderer::~Renderer() {
  if (context_) {
    context_->GetDevice().waitIdle();
  }
}

void Renderer::RenderFrame() {
  if (!isInitialized_) {
    LOG_WARNING("Renderer not initialized.");
    return;
  }

  auto imageIndexOpt = frameManager_->BeginFrame(*swapchain_);
  if (!imageIndexOpt.has_value()) {
    return;
  }

  uint32_t imageIndex = imageIndexOpt.value();
  auto cmd = frameManager_->GetCurrentCommandBuffer();

  // Transition to color attachment
  vk::ImageMemoryBarrier barrier{
      .oldLayout = vk::ImageLayout::eUndefined,
      .newLayout = vk::ImageLayout::eColorAttachmentOptimal,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = swapchain_->GetImages()[imageIndex],
      .subresourceRange = {.aspectMask = vk::ImageAspectFlagBits::eColor,
                           .baseMipLevel = 0,
                           .levelCount = 1,
                           .baseArrayLayer = 0,
                           .layerCount = 1}};
  barrier.srcAccessMask = {};
  barrier.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
  cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe,
                      vk::PipelineStageFlagBits::eColorAttachmentOutput, {}, {},
                      {}, barrier);

  // Begin rendering
  vk::RenderingAttachmentInfo colorAttachment{
      .imageView = swapchain_->GetImageViews()[imageIndex].get(),
      .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
      .loadOp = vk::AttachmentLoadOp::eClear,
      .storeOp = vk::AttachmentStoreOp::eStore,
      .clearValue = vk::ClearValue{
          .color = {std::array<float, 4>{0.1F, 0.1F, 0.1F, 1.0F}}}};

  vk::RenderingInfo renderingInfo{
      .renderArea = {.extent = swapchain_->GetExtent()},
      .layerCount = 1,
      .colorAttachmentCount = 1,
      .pColorAttachments = &colorAttachment};
  cmd.beginRendering(renderingInfo);

  vk::Viewport viewport{
      .x = 0.0F,
      .y = 0.0F,
      .width = static_cast<float>(swapchain_->GetExtent().width),
      .height = static_cast<float>(swapchain_->GetExtent().height),
      .minDepth = 0.0F,
      .maxDepth = 1.0F};
  cmd.setViewport(0, viewport);

  vk::Rect2D scissor{.offset = {.x = 0, .y = 0},
                     .extent = swapchain_->GetExtent()};
  cmd.setScissor(0, scissor);

  // Render via ECS
  renderingSystem_->Render(registry_, cmd, *basicPipeline_);

  cmd.endRendering();

  // Transition to present
  barrier.oldLayout = vk::ImageLayout::eColorAttachmentOptimal;
  barrier.newLayout = vk::ImageLayout::ePresentSrcKHR;
  barrier.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
  barrier.dstAccessMask = {};
  cmd.pipelineBarrier(vk::PipelineStageFlagBits::eColorAttachmentOutput,
                      vk::PipelineStageFlagBits::eBottomOfPipe, {}, {}, {},
                      barrier);

  frameManager_->EndFrame(*swapchain_, imageIndex);
}

void Renderer::Resize(int width, int height) {
  if (!isInitialized_) {
    LOG_WARNING("Renderer not initialized.");
    return;
  }

  LOG_DEBUG("Resizing swapchain to {}x{}...", width, height);
  swapchain_->Resize(static_cast<uint32_t>(width),
                     static_cast<uint32_t>(height));
  LOG_DEBUG("Swapchain resized successfully.");
}
}  // namespace rendering
