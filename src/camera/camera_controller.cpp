#include "camera/camera_controller.hpp"

#include <glm/gtc/matrix_transform.hpp>

namespace camera {

Camera::Camera(const CameraSettings& settings, float aspectRatio)
    : settings_{settings}, aspectRatio_{aspectRatio} {
  UpdateVectors();
  UpdateProjection();
  UpdateView();
  UpdateFrustum();
}

void Camera::SetRotation(float yaw, float pitch) {
  state_.yaw = yaw;
  state_.pitch = glm::clamp(pitch, -glm::radians(settings_.pitchLimit),
                            glm::radians(settings_.pitchLimit));
  UpdateVectors();
  UpdateView();
  UpdateFrustum();
}

void Camera::SetAspectRatio(float aspect) {
  aspectRatio_ = aspect;
  UpdateProjection();
  UpdateFrustum();
}

void Camera::UpdateVectors() {
  glm::vec3 front;
  front.x = cos(state_.pitch) * cos(state_.yaw);
  front.y = sin(state_.pitch);
  front.z = cos(state_.pitch) * sin(state_.yaw);

  forward_ = glm::normalize(front);
  right_ = glm::normalize(glm::cross(forward_, glm::vec3{0.0F, 1.0F, 0.0F}));
  up_ = glm::normalize(glm::cross(right_, forward_));
}

void Camera::UpdateView() {
  view_ = glm::lookAt(state_.position, state_.position + forward_, up_);
}

void Camera::UpdateProjection() {
  projection_ = glm::perspective(settings_.fov, aspectRatio_,
                                 settings_.nearPlane, settings_.farPlane);
  projection_[1][1] *= -1;  // Vulkan NDC Y is inverted
}

void Camera::UpdateFrustum() {
  glm::mat4 vp = projection_ * view_;
  // Gribb-Hartmann Extraction
  frustumPlanes_[0] = vp[3] + vp[0];  // Left
  frustumPlanes_[1] = vp[3] - vp[0];  // Right
  frustumPlanes_[2] = vp[3] + vp[1];  // Bottom
  frustumPlanes_[3] = vp[3] - vp[1];  // Top
  frustumPlanes_[4] = vp[3] + vp[2];  // Near
  frustumPlanes_[5] = vp[3] - vp[2];  // Far

  for (auto& plane : frustumPlanes_) {
    plane /= glm::length(glm::vec3(plane));
  }
}
}  // namespace camera
