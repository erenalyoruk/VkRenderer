#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

#include <vulkan/vulkan.hpp>

#include "vulkan/command_system.hpp"
#include "vulkan/context.hpp"
#include "vulkan/swapchain.hpp"

namespace vulkan {
constexpr uint32_t kMaxFramesInFlight{2};

struct PerFrameData {
  vk::UniqueSemaphore imageAvailableSemaphore;
  vk::UniqueFence inFlightFence;

  vk::UniqueCommandBuffer commandBuffer;

  std::vector<std::function<void()>> deletions;
};

class FrameManager {
 public:
  FrameManager(Context& context, CommandSystem& commandSystem);
  ~FrameManager();

  void EnqueueDeletion(std::function<void()> deletion);

  std::optional<uint32_t> BeginFrame(Swapchain& swapchain);

  void EndFrame(Swapchain& swapchain, uint32_t imageIndex);

  [[nodiscard]] vk::CommandBuffer GetCurrentCommandBuffer() const {
    return perFrameData_.at(currentFrameIndex_).commandBuffer.get();
  }

  [[nodiscard]] uint32_t GetCurrentFrameIndex() const {
    return currentFrameIndex_;
  }

 private:
  Context& context_;

  std::array<PerFrameData, kMaxFramesInFlight> perFrameData_;

  std::vector<vk::UniqueSemaphore> renderFinishedSemaphores_;

  uint32_t currentFrameIndex_{0};
};
}  // namespace vulkan
