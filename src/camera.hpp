#pragma once

#include <array>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include "input.hpp"

class Camera {
 public:
  Camera(Input& input, float fovRadians, float aspect, float near, float far,
         float mouseSensitivity = 0.005F);

  void Update(float dt);
  void SetAspect(float aspect);

  [[nodiscard]] glm::mat4 GetViewMatrix() const { return view_; }
  [[nodiscard]] glm::mat4 GetProjectionMatrix() const { return projection_; }
  [[nodiscard]] glm::mat4 GetViewProjectionMatrix() const {
    return projection_ * view_;
  }
  [[nodiscard]] const std::array<glm::vec4, 6>& GetFrustumPlanes() const {
    return frustumPlanes_;
  }

 private:
  void UpdateVectors();  // Centralized trig math
  void UpdateView();
  void UpdateProjection();
  void UpdateFrustum();

  Input& input_;  // NOLINT

  // State
  glm::vec3 position_{0.0F, 0.0F, 3.0F};
  float yaw_{-glm::half_pi<float>()};  // Initialized to face -Z (Forward)
  float pitch_{0.0F};

  // Cached Directions
  glm::vec3 forward_{0.0F, 0.0F, -1.0F};
  glm::vec3 right_{1.0F, 0.0F, 0.0F};
  glm::vec3 up_{0.0F, 1.0F, 0.0F};

  // Settings
  float fov_, aspect_, near_, far_;
  float mouseSensitivity_;

  // Matrices
  glm::mat4 view_{1.0F};
  glm::mat4 projection_{1.0F};
  std::array<glm::vec4, 6> frustumPlanes_{};
};
