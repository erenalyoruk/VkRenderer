#pragma once

#include <vector>

#include <vulkan/vulkan.hpp>

#include "gpu/buffer.hpp"
#include "gpu/context.hpp"
#include "gpu/material.hpp"

namespace gpu {
class MaterialPalette {
 public:
  explicit MaterialPalette(Context& context);
  ~MaterialPalette() = default;

  void AddMaterial(const Material& material);
  void UpdateBuffer();

  [[nodiscard]] vk::Buffer GetBuffer() const { return buffer_.GetBuffer(); }

  [[nodiscard]] uint32_t GetMaterialCount() const {
    return static_cast<uint32_t>(materials_.size());
  }

 private:
  Context& context_;
  std::vector<Material> materials_;
  Buffer buffer_;
};
}  // namespace gpu
