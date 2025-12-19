#pragma once

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

namespace gpu {
class Context;

class Allocator {
 public:
  explicit Allocator(const Context& context);
  ~Allocator();

  Allocator(const Allocator&) = delete;
  Allocator& operator=(const Allocator&) = delete;

  Allocator(Allocator&& other) noexcept;
  Allocator& operator=(Allocator&& other) noexcept;

  [[nodiscard]] VmaAllocator GetHandle() const { return allocator_; }

  void LogStats() const;

 private:
  VmaAllocator allocator_{VK_NULL_HANDLE};
  vk::Device device_{VK_NULL_HANDLE};
};
}  // namespace gpu
