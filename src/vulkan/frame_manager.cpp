#include "vulkan/frame_manager.hpp"

#include "logger.hpp"

namespace vulkan {
FrameManager::FrameManager(Context& context, CommandSystem& commandSystem)
    : context_{context} {
  vk::Device device{context_.GetDevice()};

  for (uint32_t i{0}; i < kMaxFramesInFlight; ++i) {
    perFrameData_.at(i).imageAvailableSemaphore =
        device.createSemaphoreUnique({});

    perFrameData_.at(i).inFlightFence = device.createFenceUnique(
        vk::FenceCreateInfo{.flags = vk::FenceCreateFlagBits::eSignaled});

    perFrameData_.at(i).commandBuffer =
        commandSystem.CreateGraphicsCommandBuffer();
  }

  LOG_DEBUG("FrameManager initialized. (Max Frames In Flight: {})",
            kMaxFramesInFlight);
}

FrameManager::~FrameManager() { context_.GetDevice().waitIdle(); }

void FrameManager::EnqueueDeletion(std::function<void()> deletion) {
  PerFrameData& frame{perFrameData_.at(currentFrameIndex_)};
  frame.deletions.push_back(std::move(deletion));
}

std::optional<uint32_t> FrameManager::BeginFrame(Swapchain& swapchain) {
  PerFrameData& frame{perFrameData_.at(currentFrameIndex_)};
  vk::Device device{context_.GetDevice()};
  auto waitResult{
      device.waitForFences(frame.inFlightFence.get(), VK_TRUE, UINT64_MAX)};

  for (auto& fn : frame.deletions) {
    fn();
  }
  frame.deletions.clear();

  try {
    auto resultValue{
        swapchain.AcquireNextImage(frame.imageAvailableSemaphore.get())};

    if (resultValue.result == vk::Result::eErrorOutOfDateKHR) {
      // Render anyway
      // return std::nullopt;
    }

    uint32_t imageIndex{resultValue.value};

    device.resetFences(frame.inFlightFence.get());

    frame.commandBuffer->reset();
    frame.commandBuffer->begin(vk::CommandBufferBeginInfo{
        .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});

    return imageIndex;
  } catch (vk::OutOfDateKHRError&) {
    LOG_WARNING("Swapchain out of date during acquire. Resizing needed!");
    return std::nullopt;
  } catch (vk::SystemError& e) {
    LOG_ERROR("Failed to acquire swapchain image: {}", e.what());
    throw;
  }
}

void FrameManager::EndFrame(Swapchain& swapchain, uint32_t imageIndex) {
  PerFrameData& frame{perFrameData_.at(currentFrameIndex_)};

  frame.commandBuffer->end();

  if (renderFinishedSemaphores_.size() != swapchain.GetImageCount()) {
    context_.GetDevice().waitIdle();
    renderFinishedSemaphores_.resize(swapchain.GetImageCount());
    for (auto& semaphore : renderFinishedSemaphores_) {
      semaphore = context_.GetDevice().createSemaphoreUnique({});
    }
  }

  vk::Semaphore signalSemaphore{renderFinishedSemaphores_.at(imageIndex).get()};

  std::array<vk::PipelineStageFlags, 1> waitStages{
      vk::PipelineStageFlagBits::eColorAttachmentOutput};

  vk::Semaphore waitSemaphore{frame.imageAvailableSemaphore.get()};
  vk::SubmitInfo submitInfo{
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = &waitSemaphore,
      .pWaitDstStageMask = waitStages.data(),
      .commandBufferCount = 1,
      .pCommandBuffers = &(frame.commandBuffer.get()),
      .signalSemaphoreCount = 1,
      .pSignalSemaphores = &signalSemaphore,
  };

  vk::Queue graphicsQueue{context_.GetGraphicsQueue()};
  graphicsQueue.submit(submitInfo, frame.inFlightFence.get());

  try {
    vk::Result result{swapchain.Present(imageIndex, signalSemaphore)};
    if (result == vk::Result::eSuboptimalKHR ||
        result == vk::Result::eErrorOutOfDateKHR) {
      LOG_WARNING("Swapchain out of date during present. Resizing needed!");
    }
  } catch (vk::OutOfDateKHRError&) {
    LOG_WARNING("Swapchain out of date during present. Resizing needed!");
  }

  currentFrameIndex_ = (currentFrameIndex_ + 1) % kMaxFramesInFlight;
}
}  // namespace vulkan
