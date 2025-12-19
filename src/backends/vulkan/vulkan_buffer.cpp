#include "backends/vulkan/vulkan_buffer.hpp"

#include "logger.hpp"

namespace backends::vulkan {
std::unique_ptr<VulkanBuffer> VulkanBuffer::Create(VulkanAllocator& allocator,
                                                   rhi::Size size,
                                                   rhi::BufferUsage usage,
                                                   rhi::MemoryUsage memUsage) {
  vk::BufferUsageFlags vkUsage{};
  VmaMemoryUsage vmaUsage{};
  VmaAllocationCreateFlags vmaFlags{};

  // Map RHI BufferUsage to Vulkan BufferUsageFlags
  if ((usage & rhi::BufferUsage::Vertex) != rhi::BufferUsage{0}) {
    vkUsage |= vk::BufferUsageFlagBits::eVertexBuffer;
  }
  if ((usage & rhi::BufferUsage::Index) != rhi::BufferUsage{0}) {
    vkUsage |= vk::BufferUsageFlagBits::eIndexBuffer;
  }
  if ((usage & rhi::BufferUsage::Uniform) != rhi::BufferUsage{0}) {
    vkUsage |= vk::BufferUsageFlagBits::eUniformBuffer;
  }
  if ((usage & rhi::BufferUsage::Storage) != rhi::BufferUsage{0}) {
    vkUsage |= vk::BufferUsageFlagBits::eStorageBuffer;
    vkUsage |= vk::BufferUsageFlagBits::eShaderDeviceAddress;
  }

  vkUsage |= vk::BufferUsageFlagBits::eTransferSrc |
             vk::BufferUsageFlagBits::eTransferDst;

  // Map RHI MemoryUsage to VMA
  switch (memUsage) {
    case rhi::MemoryUsage::GPUOnly:
      vmaUsage = VMA_MEMORY_USAGE_GPU_ONLY;
      break;
    case rhi::MemoryUsage::CPUToGPU:
      vmaUsage = VMA_MEMORY_USAGE_CPU_TO_GPU;
      vmaFlags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
      break;
    case rhi::MemoryUsage::GPUToCPU:
      vmaUsage = VMA_MEMORY_USAGE_GPU_TO_CPU;
      vmaFlags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
      break;
    default:
      LOG_ERROR("Unknown MemoryUsage");
      return nullptr;
  }

  VkBufferCreateInfo bufferInfo{
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size = size,
      .usage = static_cast<VkBufferUsageFlags>(vkUsage),
  };

  VmaAllocationCreateInfo vmaAllocInfo{
      .flags = vmaFlags,
      .usage = vmaUsage,
  };

  VkBuffer rawBuffer{};
  VmaAllocation allocation{};

  VkResult res{vmaCreateBuffer(allocator.GetHandle(), &bufferInfo,
                               &vmaAllocInfo, &rawBuffer, &allocation,
                               nullptr)};
  if (res != VK_SUCCESS) {
    LOG_ERROR("Failed to create buffer of size {}", size);
    return nullptr;
  }

  vk::DeviceAddress deviceAddress{0};
  if (vkUsage & vk::BufferUsageFlagBits::eShaderDeviceAddress) {
    vk::BufferDeviceAddressInfo addressInfo{.buffer = vk::Buffer{rawBuffer}};
    deviceAddress = allocator.GetDevice().getBufferAddress(addressInfo);
  }

  return std::unique_ptr<VulkanBuffer>(
      new VulkanBuffer(allocator, rawBuffer, allocation, size, deviceAddress));
}

VulkanBuffer::VulkanBuffer(VulkanAllocator& allocator, vk::Buffer buffer,
                           VmaAllocation allocation, rhi::Size size,
                           vk::DeviceAddress deviceAddress)
    : allocator_(&allocator),
      buffer_(buffer),
      allocation_(allocation),
      size_(size),
      deviceAddress_(deviceAddress) {}

VulkanBuffer::~VulkanBuffer() {
  if (buffer_ != nullptr && allocator_ != nullptr) {
    vmaDestroyBuffer(allocator_->GetHandle(), buffer_, allocation_);
  }
}

void* VulkanBuffer::Map() {
  void* data = nullptr;
  VkResult res = vmaMapMemory(allocator_->GetHandle(), allocation_, &data);
  if (res != VK_SUCCESS) {
    LOG_ERROR("Failed to map buffer memory!");
    return nullptr;
  }
  return data;
}

void VulkanBuffer::Unmap() {
  vmaUnmapMemory(allocator_->GetHandle(), allocation_);
}

void VulkanBuffer::Upload(std::span<const std::byte> data, rhi::Size offset) {
  rhi::Size bytes = data.size();
  if (bytes + offset > size_) {
    LOG_ERROR(
        "Buffer overflow in upload! Data size: {}, Offset: {}, Buffer size: {}",
        bytes, offset, size_);
    return;
  }

  void* dest = Map();
  if (dest != nullptr) {
    std::span<std::byte> destSpan{static_cast<std::byte*>(dest), size_};
    std::memcpy(destSpan.subspan(offset).data(), data.data(), bytes);
    Unmap();
  }
}
}  // namespace backends::vulkan
