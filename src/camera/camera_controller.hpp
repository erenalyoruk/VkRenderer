#pragma once

#include <array>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

namespace camera {
struct CameraState {
  glm::vec3 position{0.0F, 0.0F, 3.0F};
  float yaw{-glm::half_pi<float>()};
  float pitch{0.0F};
};

struct CameraSettings {
  float fov{glm::radians(60.0F)};
  float nearPlane{0.1F};
  float farPlane{100.0F};
  float movementSpeed{5.0F};
  float mouseSensitivity{0.1F};  // degrees per pixel
  float pitchLimit{89.0F};       // degrees
};

class Camera {
 public:
  Camera(const CameraSettings& settings, float aspectRatio);

  // State
  void SetPosition(const glm::vec3& position) { state_.position = position; }
  void SetRotation(float yaw, float pitch);
  void SetAspectRatio(float aspect);

  [[nodiscard]] const glm::vec3& GetPosition() const { return state_.position; }
  [[nodiscard]] float GetYaw() const { return state_.yaw; }
  [[nodiscard]] float GetPitch() const { return state_.pitch; }
  [[nodiscard]] glm::vec3 GetForward() const { return forward_; }
  [[nodiscard]] glm::vec3 GetRight() const { return right_; }
  [[nodiscard]] glm::vec3 GetUp() const { return up_; }

  // Matrices
  [[nodiscard]] const glm::mat4& GetView() const { return view_; }
  [[nodiscard]] const glm::mat4& GetProjection() const { return projection_; }
  [[nodiscard]] glm::mat4 GetViewProjection() const {
    return projection_ * view_;
  }

  // Frustum
  [[nodiscard]] const std::array<glm::vec4, 6>& GetFrustumPlanes() const {
    return frustumPlanes_;
  }

 private:
  void UpdateVectors();
  void UpdateView();
  void UpdateProjection();
  void UpdateFrustum();

  CameraState state_;
  CameraSettings settings_;
  float aspectRatio_;

  glm::vec3 forward_{0.0F, 0.0F, -1.0F};
  glm::vec3 right_{1.0F, 0.0F, 0.0F};
  glm::vec3 up_{0.0F, 1.0F, 0.0F};

  glm::mat4 view_{1.0F};
  glm::mat4 projection_{1.0F};
  std::array<glm::vec4, 6> frustumPlanes_{};
};
}  // namespace camera
