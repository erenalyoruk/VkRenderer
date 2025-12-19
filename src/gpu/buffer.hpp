#pragma once

#include <cstring>
#include <span>

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

#include "gpu/allocator.hpp"

namespace gpu {
class Buffer {
 public:
  static Buffer Create(Allocator& allocator, vk::DeviceSize size,
                       vk::BufferUsageFlags usage, VmaMemoryUsage memoryUsage,
                       VmaAllocationCreateFlags allocFlags = 0);

  Buffer() = default;
  ~Buffer();

  Buffer(const Buffer&) = delete;
  Buffer& operator=(const Buffer&) = delete;

  Buffer(Buffer&& other) noexcept;
  Buffer& operator=(Buffer&& other) noexcept;

  [[nodiscard]] vk::Buffer GetBuffer() const { return buffer_; }
  [[nodiscard]] vk::DeviceSize GetSize() const { return size_; }
  [[nodiscard]] vk::DeviceAddress GetDeviceAddress() const {
    return deviceAddress_;
  }

  void* Map();
  void Unmap();

  template <typename T>
  void Upload(std::span<const T> data, size_t offset = 0) {
    size_t bytes{data.size_bytes()};
    LOG_ASSERT(bytes + offset <= size_, "Buffer overflow in upload!");

    LOG_ASSERT(offset % alignof(T) == 0, "Offset not properly aligned!");

    void* dest{Map()};
    if (dest != nullptr) {
      std::span<std::byte> buf{static_cast<std::byte*>(dest), size_};
      std::memcpy(buf.subspan(offset).data(), data.data(), bytes);
      Unmap();
    }
  }

 private:
  Allocator* allocator_{nullptr};
  vk::Buffer buffer_{VK_NULL_HANDLE};
  VmaAllocation allocation_{VK_NULL_HANDLE};
  vk::DeviceSize size_{0};
  vk::DeviceAddress deviceAddress_{0};

  Buffer(Allocator& allocator, vk::Buffer buffer, VmaAllocation allocation,
         vk::DeviceSize size, vk::DeviceAddress deviceAddress);
};
}  // namespace gpu
