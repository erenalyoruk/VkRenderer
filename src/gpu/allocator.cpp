// Define VMA_IMPLEMENTATION before including the header to create the
// implementation.
#define VMA_IMPLEMENTATION

// Disable VMA's internal fetch of Vulkan function pointers since we are
// providing them ourselves.
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0

#include "gpu/allocator.hpp"

#include <utility>

#include "logger.hpp"

#define MAP_VMA_TO_VULKAN(fn) .fn = VULKAN_HPP_DEFAULT_DISPATCHER.fn

namespace gpu {
Allocator::Allocator(const Context& context) : device_{context.GetDevice()} {
  VmaVulkanFunctions vulkanFns{
      // Instance level functions
      MAP_VMA_TO_VULKAN(vkGetInstanceProcAddr),
      MAP_VMA_TO_VULKAN(vkGetDeviceProcAddr),

      // Device level functions
      MAP_VMA_TO_VULKAN(vkGetPhysicalDeviceProperties),
      MAP_VMA_TO_VULKAN(vkGetPhysicalDeviceMemoryProperties),
      MAP_VMA_TO_VULKAN(vkAllocateMemory), MAP_VMA_TO_VULKAN(vkFreeMemory),
      MAP_VMA_TO_VULKAN(vkMapMemory), MAP_VMA_TO_VULKAN(vkUnmapMemory),
      MAP_VMA_TO_VULKAN(vkFlushMappedMemoryRanges),
      MAP_VMA_TO_VULKAN(vkInvalidateMappedMemoryRanges),
      MAP_VMA_TO_VULKAN(vkBindBufferMemory),
      MAP_VMA_TO_VULKAN(vkBindImageMemory),
      MAP_VMA_TO_VULKAN(vkGetBufferMemoryRequirements),
      MAP_VMA_TO_VULKAN(vkGetImageMemoryRequirements),
      MAP_VMA_TO_VULKAN(vkCreateBuffer), MAP_VMA_TO_VULKAN(vkDestroyBuffer),
      MAP_VMA_TO_VULKAN(vkCreateImage), MAP_VMA_TO_VULKAN(vkDestroyImage),
      MAP_VMA_TO_VULKAN(vkCmdCopyBuffer),

      // VMA 3.0+ functions
      .vkGetBufferMemoryRequirements2KHR =
          VULKAN_HPP_DEFAULT_DISPATCHER.vkGetBufferMemoryRequirements2,
      .vkGetImageMemoryRequirements2KHR =
          VULKAN_HPP_DEFAULT_DISPATCHER.vkGetImageMemoryRequirements2,
      .vkBindBufferMemory2KHR =
          VULKAN_HPP_DEFAULT_DISPATCHER.vkBindBufferMemory2,
      .vkBindImageMemory2KHR = VULKAN_HPP_DEFAULT_DISPATCHER.vkBindImageMemory2,
      .vkGetPhysicalDeviceMemoryProperties2KHR =
          VULKAN_HPP_DEFAULT_DISPATCHER.vkGetPhysicalDeviceMemoryProperties2};

  VmaAllocatorCreateInfo allocInfo{
      .flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
      .physicalDevice = context.GetPhysicalDevice(),
      .device = context.GetDevice(),
      .pVulkanFunctions = &vulkanFns,
      .instance = context.GetInstance(),
      .vulkanApiVersion = VK_API_VERSION_1_3,
  };

  VkResult result{vmaCreateAllocator(&allocInfo, &allocator_)};

  LOG_DEBUG("VulkanMemoryAllocator created successfully.");
}

Allocator::~Allocator() {
  if (allocator_ != nullptr) {
    char* stats{nullptr};
    vmaBuildStatsString(allocator_, &stats, VK_TRUE);
    if (stats != nullptr) {
      LOG_DEBUG("VMA Stats on shutdown: {}", stats);
      vmaFreeStatsString(allocator_, stats);
    }

    vmaDestroyAllocator(allocator_);
  }
}

Allocator::Allocator(Allocator&& other) noexcept
    : allocator_{std::exchange(other.allocator_, VK_NULL_HANDLE)},
      device_{std::exchange(other.device_, VK_NULL_HANDLE)} {}

Allocator& Allocator::operator=(Allocator&& other) noexcept {
  if (this != &other) {
    if (allocator_ != nullptr) {
      vmaDestroyAllocator(allocator_);
    }

    allocator_ = std::exchange(other.allocator_, VK_NULL_HANDLE);
    device_ = std::exchange(other.device_, VK_NULL_HANDLE);
  }

  return *this;
}

void Allocator::LogStats() const {
  char* stats{nullptr};
  vmaBuildStatsString(allocator_, &stats, VK_TRUE);
  if (stats != nullptr) {
    LOG_DEBUG("VMA Stats: {}", stats);
    vmaFreeStatsString(allocator_, stats);
  }
}
}  // namespace gpu

#undef MAP_VMA_TO_VULKAN
