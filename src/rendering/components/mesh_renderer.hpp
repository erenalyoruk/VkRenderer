#pragma once

#include <memory>

#include "vulkan/mesh.hpp"

struct MeshRenderer {
  std::shared_ptr<vulkan::Mesh> mesh;
  uint32_t materialIndex{0};

  MeshRenderer(std::shared_ptr<vulkan::Mesh> mesh, uint32_t materialIndex = 0)
      : mesh{std::move(mesh)}, materialIndex{materialIndex} {}
};
