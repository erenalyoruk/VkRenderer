#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace rendering {
struct Transform {
  glm::vec3 position{0.0F};
  glm::quat rotation{glm::identity<glm::quat>()};
  glm::vec3 scale{1.0F};

  Transform(glm::vec3 pos, glm::quat rot, glm::vec3 scl)
      : position{pos}, rotation{rot}, scale{scl} {}

  [[nodiscard]] glm::mat4 GetMatrix() const {
    return glm::translate(glm::mat4{1.0F}, position) *
           glm::mat4_cast(rotation) * glm::scale(glm::mat4(1.0F), scale);
  }
};
}  // namespace rendering
