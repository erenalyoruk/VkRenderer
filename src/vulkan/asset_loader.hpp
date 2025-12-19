#pragma once

#include "vulkan/context.hpp"
#include "vulkan/mesh.hpp"

namespace vulkan {
class AssetLoader {
 public:
  explicit AssetLoader(Context& context);

  [[nodiscard]] Mesh LoadMesh(const std::string& filepath);

 private:
  Context& context_;
};
}  // namespace vulkan
