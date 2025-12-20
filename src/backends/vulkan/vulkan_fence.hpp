#pragma once

#include <memory>

#include <vulkan/vulkan.hpp>

#include "rhi/sync.hpp"

namespace backends::vulkan {
class VulkanContext;

class VulkanFence : public rhi::Fence {
 public:
  static std::unique_ptr<VulkanFence> Create(VulkanContext& context,
                                             bool signaled = false);

  // RHI implementations
  void Wait(uint64_t timeout = UINT64_MAX) override;
  void Reset() override;
  [[nodiscard]] bool IsSignaled() const override;

  // Vulkan getter
  [[nodiscard]] vk::Fence GetFence() const { return fence_.get(); }

 private:
  VulkanFence(vk::Device device, vk::UniqueFence fence);

  vk::Device device_;
  vk::UniqueFence fence_;
};
}  // namespace backends::vulkan
