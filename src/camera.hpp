#pragma once

#include <array>

#include <glm/glm.hpp>

#include "input.hpp"

class Camera {
 public:
  Camera(Input& input, float fovRadians, float aspect, float near, float far);

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
  Input& input_;
  glm::vec3 position_{0.0F, 0.0F, 3.0F};
  float yaw_{0.0F};
  float pitch_{0.0F};
  float fov_{0.0F};
  float aspect_{0.0F};
  float near_{0.0F};
  float far_{0.0F};
  glm::mat4 view_{1.0F};
  glm::mat4 projection_{1.0F};
  std::array<glm::vec4, 6> frustumPlanes_{};

  void UpdateView();
  void UpdateProjection();
  void UpdateFrustum();
};
