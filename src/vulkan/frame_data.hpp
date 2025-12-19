#pragma once

#include <glm/glm.hpp>

struct FrameData {
  glm::mat4 viewProj;      // View-projection matrix
  glm::vec4 ambientColor;  // Ambient light color (vec4 for alignment)
  glm::vec4 sunDirection;  // Sun direction (vec4 for 16-byte alignment)
};
