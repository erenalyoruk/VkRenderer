#include "gpu/global_shader_data.hpp"

namespace gpu {
GlobalShaderData::GlobalShaderData(Context& context)
    : context_{context},
      buffer_{Buffer::Create(
          context.GetAllocator(), sizeof(FrameData),
          vk::BufferUsageFlagBits::eUniformBuffer, VMA_MEMORY_USAGE_AUTO,
          VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
              VMA_ALLOCATION_CREATE_MAPPED_BIT)} {}

void GlobalShaderData::Update(const FrameData& data) {
  buffer_.Upload(std::span<const FrameData>(&data, 1));
}
}  // namespace gpu
