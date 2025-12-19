#pragma once

#include <vulkan/vulkan.hpp>

#include "gpu/buffer.hpp"
#include "gpu/context.hpp"
#include "gpu/frame_data.hpp"

namespace gpu {
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
}  // namespace gpu
