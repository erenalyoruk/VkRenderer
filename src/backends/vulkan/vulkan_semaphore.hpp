#pragma once

#include <memory>

#include <vulkan/vulkan.hpp>

#include "rhi/sync.hpp"

namespace backends::vulkan {
class VulkanContext;

class VulkanSemaphore : public rhi::Semaphore {
 public:
  static std::unique_ptr<VulkanSemaphore> Create(VulkanContext& context);

  // Vulkan getter
  [[nodiscard]] vk::Semaphore GetSemaphore() const { return semaphore_.get(); }

 private:
  VulkanSemaphore(vk::UniqueSemaphore semaphore);

  vk::UniqueSemaphore semaphore_;
};
}  // namespace backends::vulkan
