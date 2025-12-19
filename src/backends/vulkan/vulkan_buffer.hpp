#pragma once
#pragma once

#include <memory>

#include "backends/vulkan/vulkan_allocator.hpp"
#include "rhi/buffer.hpp"

namespace backends::vulkan {
class VulkanBuffer : public rhi::Buffer {
 public:
  ~VulkanBuffer() override;

  static std::unique_ptr<VulkanBuffer> Create(VulkanAllocator& allocator,
                                              rhi::Size size,
                                              rhi::BufferUsage usage,
                                              rhi::MemoryUsage memUsage);

  // RHI implementations
  void* Map() override;
  void Unmap() override;
  void Upload(std::span<const std::byte> data, rhi::Size offset = 0) override;

  [[nodiscard]] rhi::Size GetSize() const override { return size_; }
  [[nodiscard]] rhi::Address GetDeviceAddress() const override {
    return deviceAddress_;
  }

 private:
  VulkanBuffer(VulkanAllocator& allocator, vk::Buffer buffer,
               VmaAllocation allocation, rhi::Size size,
               vk::DeviceAddress deviceAddress);
  // Members
  VulkanAllocator* allocator_;
  vk::Buffer buffer_;
  VmaAllocation allocation_;
  rhi::Size size_;
  vk::DeviceAddress deviceAddress_;
};
}  // namespace backends::vulkan
