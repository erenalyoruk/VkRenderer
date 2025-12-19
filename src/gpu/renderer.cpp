#include "gpu/renderer.hpp"

#include <array>

#include "logger.hpp"

namespace gpu {
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
  swapchain_->Resize(width, height);
  LOG_DEBUG("Swapchain resized successfully.");
}

void Renderer::RenderFrame() {
  if (!isInitialized_) {
    return;
  }

  auto imageIndexOpt = frameManager_->BeginFrame(*swapchain_);
  if (!imageIndexOpt) {
    // Swapchain out of date; skip frame
    return;
  }

  uint32_t imageIndex = *imageIndexOpt;
  vk::CommandBuffer cmd = frameManager_->GetCurrentCommandBuffer();
  vk::Image image = swapchain_->GetImages()[imageIndex];

  // Transition image to transfer destination layout
  vk::ImageMemoryBarrier barrier{
      .oldLayout = vk::ImageLayout::eUndefined,
      .newLayout = vk::ImageLayout::eTransferDstOptimal,
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
                      vk::PipelineStageFlagBits::eTransfer, {}, {}, {},
                      barrier);

  // Clear image to black
  vk::ClearColorValue clearColor{std::array<float, 4>{0.2F, 0.4F, 0.2F, 1.0F}};
  cmd.clearColorImage(image, vk::ImageLayout::eTransferDstOptimal, clearColor,
                      {barrier.subresourceRange});

  // Transition image to present source layout
  barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
  barrier.newLayout = vk::ImageLayout::ePresentSrcKHR;
  barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
  cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                      vk::PipelineStageFlagBits::eBottomOfPipe, {}, {}, {},
                      barrier);

  frameManager_->EndFrame(*swapchain_, imageIndex);
}
}  // namespace gpu
