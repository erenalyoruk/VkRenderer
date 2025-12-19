#pragma once

#include <vulkan/vulkan.hpp>

#include "vulkan/buffer.hpp"
#include "vulkan/context.hpp"
#include "vulkan/frame_data.hpp"

namespace vulkan {
class GlobalShaderData {
 public:
  explicit GlobalShaderData(Context& context);
  ~GlobalShaderData() = default;

  void Update(const FrameData& data);

  [[nodiscard]] const Buffer& GetBuffer() const { return buffer_; }

 private:
  Context& context_;
  Buffer buffer_;
};
}  // namespace vulkan
