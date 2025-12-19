#include "gpu/material_palette.hpp"

#include <span>

namespace gpu {
MaterialPalette::MaterialPalette(Context& context) : context_{context} {}

void MaterialPalette::AddMaterial(const Material& material) {
  materials_.push_back(material);
  UpdateBuffer();
}

void MaterialPalette::UpdateBuffer() {
  if (materials_.empty()) {
    return;
  }

  buffer_ = Buffer::Create(
      context_.GetAllocator(), materials_.size() * sizeof(Material),
      vk::BufferUsageFlagBits::eStorageBuffer, VMA_MEMORY_USAGE_AUTO,
      VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
          VMA_ALLOCATION_CREATE_MAPPED_BIT);

  buffer_.Upload(
      std::span<const Material>(materials_.data(), materials_.size()));
}
}  // namespace gpu
