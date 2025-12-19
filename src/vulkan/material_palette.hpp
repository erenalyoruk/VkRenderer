#pragma once

#include <vector>

#include <vulkan/vulkan.hpp>

#include "vulkan/buffer.hpp"
#include "vulkan/context.hpp"
#include "vulkan/material.hpp"

namespace vulkan {
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
}  // namespace vulkan
