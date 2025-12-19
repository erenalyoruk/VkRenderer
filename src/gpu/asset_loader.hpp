#pragma once

#include "gpu/context.hpp"
#include "gpu/mesh.hpp"

namespace gpu {
class AssetLoader {
 public:
  explicit AssetLoader(Context& context);

  [[nodiscard]] Mesh LoadMesh(const std::string& filepath);

 private:
  Context& context_;
};
}  // namespace gpu
