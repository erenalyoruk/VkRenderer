#pragma once

#include <array>
#include <vector>

#include <glm/glm.hpp>

#include "ecs/components.hpp"

namespace renderer {

/**
 * @brief Generate a simple cube mesh for skybox rendering
 */
inline std::pair<std::vector<ecs::Vertex>, std::vector<uint32_t>>
GenerateCubeMesh() {
  // Cube vertices (just positions needed for skybox)
  std::array<glm::vec3, 8> positions = {{
      {-1.0F, -1.0F, -1.0F},
      {1.0F, -1.0F, -1.0F},
      {1.0F, 1.0F, -1.0F},
      {-1.0F, 1.0F, -1.0F},
      {-1.0F, -1.0F, 1.0F},
      {1.0F, -1.0F, 1.0F},
      {1.0F, 1.0F, 1.0F},
      {-1.0F, 1.0F, 1.0F},
  }};

  // Cube indices (36 indices for 12 triangles)
  std::vector<uint32_t> indices{
      // Front face
      0,
      1,
      2,
      2,
      3,
      0,

      // Back face
      4,
      6,
      5,
      6,
      4,
      7,

      // Top face
      3,
      2,
      6,
      6,
      7,
      3,

      // Bottom face
      0,
      5,
      1,
      5,
      0,
      4,
      // Right face
      1,
      5,
      6,
      6,
      2,
      1,
      // Left face
      0,
      3,
      7,
      7,
      4,
      0,
  };

  std::vector<ecs::Vertex> vertices;
  vertices.reserve(8);

  for (const auto& pos : positions) {
    ecs::Vertex v{};
    v.position = pos;
    v.normal = glm::normalize(pos);  // For skybox, normal points outward
    v.color = glm::vec4(1.0F);
    vertices.push_back(v);
  }

  return {vertices, indices};
}

}  // namespace renderer
