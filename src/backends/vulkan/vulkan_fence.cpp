#include "backends/vulkan/vulkan_fence.hpp"

#include <utility>

#include "backends/vulkan/vulkan_context.hpp"

namespace backends::vulkan {
std::unique_ptr<VulkanFence> VulkanFence::Create(VulkanContext& context,
                                                 bool signaled) {
  vk::FenceCreateInfo createInfo{
      .flags = signaled ? vk::FenceCreateFlagBits::eSignaled
                        : vk::FenceCreateFlags{},
  };

  vk::UniqueFence fence{context.GetDevice().createFenceUnique(createInfo)};

  return std::unique_ptr<VulkanFence>(
      new VulkanFence(context.GetDevice(), std::move(fence)));
}

VulkanFence::VulkanFence(vk::Device device, vk::UniqueFence fence)
    : device_{device}, fence_{std::move(fence)} {}

void VulkanFence::Wait(uint64_t timeout) {
  [[maybe_unused]] vk::Result result =
      device_.waitForFences(fence_.get(), VK_TRUE, timeout);
}

void VulkanFence::Reset() { device_.resetFences(fence_.get()); }

bool VulkanFence::IsSignaled() const {
  return device_.getFenceStatus(fence_.get()) == vk::Result::eSuccess;
}
}  // namespace backends::vulkan
