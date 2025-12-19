#include "vulkan/renderer.hpp"

#include <array>

#include "logger.hpp"
#include "vulkan/frame_data.hpp"
#include "vulkan/pipeline.hpp"
#include "vulkan/shader.hpp"

namespace vulkan {
Renderer::Renderer(Window& window, bool enableValidationLayers)
    : window_{window}, enableValidationLayers_{enableValidationLayers} {
  if (isInitialized_) {
    LOG_WARNING("Renderer already initialized.");
    return;
  }

  LOG_DEBUG("Initializing Renderer...");

  context_ = std::make_unique<Context>(window_, enableValidationLayers_);
  commandSystem_ = std::make_unique<CommandSystem>(*context_);
  swapchain_ = std::make_unique<Swapchain>(
      *context_, static_cast<uint32_t>(window_.GetWidth()),
      static_cast<uint32_t>(window_.GetHeight()));
  frameManager_ = std::make_unique<FrameManager>(*context_, *commandSystem_);
  globalShaderData_ = std::make_unique<GlobalShaderData>(*context_);
  materialPalette_ = std::make_unique<MaterialPalette>(*context_);

  Material defaultMat{.diffuseIndex = 0, .normalIndex = 1};
  materialPalette_->AddMaterial(defaultMat);

  camera_ =
      std::make_unique<Camera>(window_.GetInput(), glm::radians(45.0F),
                               static_cast<float>(window_.GetWidth()) /
                                   static_cast<float>(window_.GetHeight()),
                               0.1F, 100.0F);

  assetLoader_ = std::make_unique<AssetLoader>(*context_);  // Add this
  testMesh_ = std::make_unique<Mesh>(assetLoader_->LoadMesh(
      "assets/models/microphone.gltf"));  // Add this (replace with actual path)

  // Descriptor setup
  DescriptorLayoutBuilder layoutBuilder;
  layoutBuilder.AddBinding(0, vk::DescriptorType::eUniformBuffer,
                           vk::ShaderStageFlagBits::eVertex);
  layoutBuilder.AddBinding(1, vk::DescriptorType::eStorageBuffer,
                           vk::ShaderStageFlagBits::eVertex);
  descriptorSetLayout_ = layoutBuilder.Build(context_->GetDevice());

  std::vector<DescriptorAllocator::PoolSizeRatio> poolRatios{
      {.type = vk::DescriptorType::eUniformBuffer, .ratio = 1},
      {.type = vk::DescriptorType::eStorageBuffer, .ratio = 1},
  };
  descriptorAllocator_ = std::make_unique<DescriptorAllocator>(
      context_->GetDevice(), 1, poolRatios);
  descriptorSet_ = descriptorAllocator_->Allocate(*descriptorSetLayout_);

  DescriptorWriter writer;
  writer.WriteBuffer(0, vk::DescriptorType::eUniformBuffer,
                     {
                         .buffer = globalShaderData_->GetBuffer().GetBuffer(),
                         .offset = 0,
                         .range = sizeof(FrameData),
                     });
  writer.WriteBuffer(1, vk::DescriptorType::eStorageBuffer,
                     {
                         .buffer = materialPalette_->GetBuffer(),
                         .offset = 0,
                         .range = VK_WHOLE_SIZE,
                     });
  writer.Overwrite(context_->GetDevice(), descriptorSet_);

  pipelineLayout_ = context_->GetDevice().createPipelineLayoutUnique({
      .setLayoutCount = 1,
      .pSetLayouts = &descriptorSetLayout_.get(),
  });

  vertShader_ = std::make_unique<vk::UniqueShaderModule>(
      LoadShader(context_->GetDevice(), "assets/shaders/simple.vert.spv"));
  fragShader_ = std::make_unique<vk::UniqueShaderModule>(
      LoadShader(context_->GetDevice(), "assets/shaders/simple.frag.spv"));

  // Vertex input
  vk::VertexInputBindingDescription binding{
      .binding = 0,
      .stride = sizeof(Vertex),
      .inputRate = vk::VertexInputRate::eVertex,
  };
  std::vector<vk::VertexInputAttributeDescription> attributes{
      {
          .location = 0,
          .binding = 0,
          .format = vk::Format::eR32G32B32Sfloat,
          .offset = offsetof(Vertex, position),
      },
      {
          .location = 1,
          .binding = 0,
          .format = vk::Format::eR32G32B32Sfloat,
          .offset = offsetof(Vertex, normal),
      },
      {
          .location = 2,
          .binding = 0,
          .format = vk::Format::eR32G32Sfloat,
          .offset = offsetof(Vertex, texCoord),
      },
  };

  pipeline_ =
      GraphicsPipelineBuilder(*pipelineLayout_, swapchain_->GetImageFormat())
          .SetShaders(vertShader_->get(), fragShader_->get())
          .SetVertexInput({binding}, attributes)
          .SetTopology(vk::PrimitiveTopology::eTriangleList)
          .SetCullMode(vk::CullModeFlagBits::eBack,
                       vk::FrontFace::eCounterClockwise)
          .SetDepthTest(false, false, vk::CompareOp::eLess)
          .Build(context_->GetDevice());

  window_.SetOnResize(
      [this](int width, int height) { this->Resize(width, height); });

  isInitialized_ = true;
  LOG_DEBUG("Renderer initialized successfully.");
}

Renderer::~Renderer() {
  if (!isInitialized_) {
    return;
  }

  LOG_DEBUG("Shutting down Renderer...");

  // Ensure device is idle before cleanup
  if (context_) {
    context_->GetDevice().waitIdle();
  }

  assetLoader_.reset();
  testMesh_.reset();
  descriptorAllocator_.reset();
  descriptorSetLayout_.reset();
  fragShader_.reset();
  vertShader_.reset();
  pipelineLayout_.reset();
  pipeline_.reset();
  camera_.reset();
  materialPalette_.reset();
  globalShaderData_.reset();
  frameManager_.reset();
  swapchain_.reset();
  commandSystem_.reset();
  context_.reset();

  isInitialized_ = false;
  LOG_DEBUG("Renderer shut down.");
}

void Renderer::Resize(int width, int height) {
  if (!isInitialized_) {
    return;
  }

  LOG_DEBUG("Resizing swapchain to {}x{}...", width, height);
  context_->GetDevice().waitIdle();
  camera_->SetAspect(static_cast<float>(width) / static_cast<float>(height));
  swapchain_->Resize(width, height);
  LOG_DEBUG("Swapchain resized successfully.");
}

void Renderer::RenderFrame() {
  if (!isInitialized_) {
    return;
  }

  camera_->Update(1.0F / 60.0F);

  FrameData frameData{
      .viewProj = camera_->GetViewProjectionMatrix(),
      .ambientColor = glm::vec4(0.1F, 0.1F, 0.1F, 1.0F),
      .sunDirection = glm::vec4(0.0F, -1.0F, 0.0F, 0.0F),
  };
  globalShaderData_->Update(frameData);

  auto imageIndexOpt = frameManager_->BeginFrame(*swapchain_);
  if (!imageIndexOpt) {
    // Swapchain out of date; skip frame
    return;
  }

  uint32_t imageIndex = *imageIndexOpt;
  vk::CommandBuffer cmd = frameManager_->GetCurrentCommandBuffer();
  vk::Image image = swapchain_->GetImages()[imageIndex];
  vk::ImageView imageView = swapchain_->GetImageViews()[imageIndex].get();

  // Transition to color attachment
  vk::ImageMemoryBarrier barrier{
      .oldLayout = vk::ImageLayout::eUndefined,
      .newLayout = vk::ImageLayout::eColorAttachmentOptimal,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = image,
      .subresourceRange =
          {
              .aspectMask = vk::ImageAspectFlagBits::eColor,
              .baseMipLevel = 0,
              .levelCount = 1,
              .baseArrayLayer = 0,
              .layerCount = 1,
          },
  };
  cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe,
                      vk::PipelineStageFlagBits::eColorAttachmentOutput, {}, {},
                      {}, barrier);

  // Begin dynamic rendering
  vk::RenderingAttachmentInfo colorAttachment{
      .imageView = imageView,
      .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
      .loadOp = vk::AttachmentLoadOp::eClear,
      .storeOp = vk::AttachmentStoreOp::eStore,
      .clearValue =
          vk::ClearValue{std::array<float, 4>{0.2F, 0.4F, 0.2F, 1.0F}},
  };
  cmd.beginRendering(vk::RenderingInfo{
      .renderArea = {.offset = {.x = 0, .y = 0},
                     .extent = swapchain_->GetExtent()},
      .layerCount = 1,
      .colorAttachmentCount = 1,
      .pColorAttachments = &colorAttachment,
  });

  // Set dynamic state
  cmd.setViewport(
      0,
      vk::Viewport{.x = 0.0F,
                   .y = 0.0F,
                   .width = static_cast<float>(swapchain_->GetExtent().width),
                   .height = static_cast<float>(swapchain_->GetExtent().height),
                   .minDepth = 0.0F,
                   .maxDepth = 1.0F});
  cmd.setScissor(0, vk::Rect2D{.offset = {.x = 0, .y = 0},
                               .extent = swapchain_->GetExtent()});

  // Bind pipeline and descriptors
  cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipeline_);
  cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipelineLayout_, 0,
                         descriptorSet_, {});

  // Bind and draw mesh
  cmd.bindVertexBuffers(0, testMesh_->vertexBuffer.GetBuffer(), {0});
  cmd.bindIndexBuffer(testMesh_->indexBuffer.GetBuffer(), 0,
                      vk::IndexType::eUint32);
  cmd.drawIndexed(testMesh_->indexCount, 1, 0, 0, 0);

  cmd.endRendering();

  // Transition to present
  barrier.oldLayout = vk::ImageLayout::eColorAttachmentOptimal;
  barrier.newLayout = vk::ImageLayout::ePresentSrcKHR;
  barrier.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
  cmd.pipelineBarrier(vk::PipelineStageFlagBits::eColorAttachmentOutput,
                      vk::PipelineStageFlagBits::eBottomOfPipe, {}, {}, {},
                      barrier);

  frameManager_->EndFrame(*swapchain_, imageIndex);
}
}  // namespace vulkan
