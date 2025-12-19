#pragma once

#include <array>
#include <cstdint>
#include <optional>

#include <vulkan/vulkan.hpp>

#include "gpu/command_system.hpp"
#include "gpu/context.hpp"
#include "gpu/swapchain.hpp"

namespace gpu {
constexpr uint32_t kMaxFramesInFlight{2};

struct PerFrameData {
  vk::UniqueSemaphore imageAvailableSemaphore;
  vk::UniqueFence inFlightFence;

  vk::UniqueCommandBuffer commandBuffer;
};

class FrameManager {
 public:
  FrameManager(Context& context, CommandSystem& commandSystem);
  ~FrameManager();

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
}  // namespace gpu
