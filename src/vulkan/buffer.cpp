#include "vulkan/buffer.hpp"

#include <utility>

#include "logger.hpp"

namespace vulkan {
Buffer Buffer::Create(Allocator& allocator, vk::DeviceSize size,
                      vk::BufferUsageFlags usage, VmaMemoryUsage memoryUsage,
                      VmaAllocationCreateFlags allocFlags) {
  VkBufferCreateInfo bufferInfo{
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size = size,
      .usage = static_cast<VkBufferUsageFlags>(usage),
  };

  VmaAllocationCreateInfo vmaAllocInfo{
      .flags = allocFlags,
      .usage = memoryUsage,
  };

  VkBuffer rawBuffer{};
  VmaAllocation allocation{};

  VkResult res{vmaCreateBuffer(allocator.GetHandle(), &bufferInfo,
                               &vmaAllocInfo, &rawBuffer, &allocation,
                               nullptr)};

  vk::DeviceAddress deviceAddress{};
  if (usage & vk::BufferUsageFlagBits::eShaderDeviceAddress) {
    vk::BufferDeviceAddressInfo addressInfo{
        .buffer = vk::Buffer{rawBuffer},
    };

    deviceAddress = allocator.GetDevice().getBufferAddress(addressInfo);
  }

  return {allocator, rawBuffer, allocation, size, deviceAddress};
}

Buffer::Buffer(Allocator& allocator, vk::Buffer buffer,
               VmaAllocation allocation, vk::DeviceSize size,
               vk::DeviceAddress deviceAddress)
    : allocator_{&allocator},
      buffer_{buffer},
      allocation_{allocation},
      size_{size},
      deviceAddress_{deviceAddress} {}

Buffer::~Buffer() {
  if (buffer_ != nullptr && allocator_ != nullptr) {
    vmaDestroyBuffer(allocator_->GetHandle(), buffer_, allocation_);
  }
}

Buffer::Buffer(Buffer&& other) noexcept
    : allocator_{std::exchange(other.allocator_, nullptr)},
      buffer_{std::exchange(other.buffer_, nullptr)},
      allocation_{std::exchange(other.allocation_, nullptr)},
      size_{std::exchange(other.size_, 0)},
      deviceAddress_{std::exchange(other.deviceAddress_, 0)} {}

Buffer& Buffer::operator=(Buffer&& other) noexcept {
  if (this != &other) {
    if (buffer_ != nullptr && allocator_ != nullptr) {
      vmaDestroyBuffer(allocator_->GetHandle(), buffer_, allocation_);
    }

    allocator_ = std::exchange(other.allocator_, nullptr);
    buffer_ = std::exchange(other.buffer_, nullptr);
    allocation_ = std::exchange(other.allocation_, nullptr);
    size_ = std::exchange(other.size_, 0);
    deviceAddress_ = std::exchange(other.deviceAddress_, 0);
  }

  return *this;
}

void* Buffer::Map() {
  void* data{nullptr};

  VkResult res{vmaMapMemory(allocator_->GetHandle(), allocation_, &data)};
  if (res != VK_SUCCESS) {
    LOG_ERROR("Failed to map buffer memory!");
    return nullptr;
  }

  return data;
}

void Buffer::Unmap() { vmaUnmapMemory(allocator_->GetHandle(), allocation_); }
}  // namespace vulkan
