#include "backends/vulkan/vulkan_semaphore.hpp"

#include <utility>

#include "backends/vulkan/vulkan_context.hpp"

namespace backends::vulkan {
std::unique_ptr<VulkanSemaphore> VulkanSemaphore::Create(
    VulkanContext& context) {
  vk::SemaphoreCreateInfo createInfo{};

  vk::UniqueSemaphore semaphore{
      context.GetDevice().createSemaphoreUnique(createInfo)};

  return std::unique_ptr<VulkanSemaphore>(
      new VulkanSemaphore(std::move(semaphore)));
}

VulkanSemaphore::VulkanSemaphore(vk::UniqueSemaphore semaphore)
    : semaphore_{std::move(semaphore)} {}
}  // namespace backends::vulkan
