#pragma once

#include <cstdint>

#include <glm/glm.hpp>

#include "vulkan/buffer.hpp"

namespace vulkan {
struct Vertex {
  glm::vec3 position;
  glm::vec3 normal;
  glm::vec2 texCoord;
};

struct Mesh {
  Buffer vertexBuffer;
  Buffer indexBuffer;
  uint32_t indexCount;
};
}  // namespace vulkan
