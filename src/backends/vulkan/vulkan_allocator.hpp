#pragma once

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

namespace backends::vulkan {
class VulkanContext;

class VulkanAllocator {
 public:
  explicit VulkanAllocator(const VulkanContext& context);
  ~VulkanAllocator();

  VulkanAllocator(const VulkanAllocator&) = delete;
  VulkanAllocator& operator=(const VulkanAllocator&) = delete;

  VulkanAllocator(VulkanAllocator&& other) noexcept;
  VulkanAllocator& operator=(VulkanAllocator&& other) noexcept;

  void LogStats() const;

  [[nodiscard]] VmaAllocator GetHandle() const { return allocator_; }
  [[nodiscard]] vk::Device GetDevice() const { return device_; }

 private:
  VmaAllocator allocator_{VK_NULL_HANDLE};
  vk::Device device_{VK_NULL_HANDLE};
};
}  // namespace backends::vulkan
