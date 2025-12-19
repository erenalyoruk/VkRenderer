#include "camera.hpp"

#include <glm/gtc/matrix_transform.hpp>

Camera::Camera(Input& input, float fovRadians, float aspect, float near,
               float far, float mouseSensitivity)
    : input_{input},
      fov_{fovRadians},
      aspect_{aspect},
      near_{near},
      far_{far},
      mouseSensitivity_{mouseSensitivity} {
  UpdateVectors();  // Establishes initial forward/right/up
  UpdateProjection();
  UpdateView();
  UpdateFrustum();
}

void Camera::UpdateVectors() {
  // Standard FPS Camera Trigonometry
  glm::vec3 front;
  front.x = cos(pitch_) * cos(yaw_);
  front.y = sin(pitch_);
  front.z = cos(pitch_) * sin(yaw_);

  forward_ = glm::normalize(front);
  right_ = glm::normalize(glm::cross(forward_, glm::vec3{0.0F, 1.0F, 0.0F}));
  up_ = glm::normalize(glm::cross(right_, forward_));
}

void Camera::Update(float dt) {
  // 1. Handle Mouse Rotation
  if (input_.IsMouseButtonDown(SDL_BUTTON_RIGHT)) {
    glm::vec2 mouseDelta = input_.GetMouseDelta();
    yaw_ += mouseDelta.x * mouseSensitivity_;
    pitch_ -= mouseDelta.y * mouseSensitivity_;  // Up move = Up look

    pitch_ = glm::clamp(pitch_, -glm::half_pi<float>() + 0.1F,
                        glm::half_pi<float>() - 0.1F);

    UpdateVectors();  // Refresh vectors after rotation
  }

  // 2. Handle Movement
  float speed = 5.0F * dt;
  if (input_.IsKeyDown(SDL_SCANCODE_W)) {
    position_ += forward_ * speed;
  }
  if (input_.IsKeyDown(SDL_SCANCODE_S)) {
    position_ -= forward_ * speed;
  }
  if (input_.IsKeyDown(SDL_SCANCODE_A)) {
    position_ -= right_ * speed;
  }
  if (input_.IsKeyDown(SDL_SCANCODE_D)) {
    position_ += right_ * speed;
  }

  constexpr glm::vec3 kWorldUp{0.0F, 1.0F, 0.0F};
  if (input_.IsKeyDown(SDL_SCANCODE_SPACE)) {
    position_ += kWorldUp * speed;
  }
  if (input_.IsKeyDown(SDL_SCANCODE_LSHIFT)) {
    position_ -= kWorldUp * speed;
  }

  // 3. Finalize
  UpdateView();
  UpdateFrustum();
}

void Camera::UpdateView() {
  view_ = glm::lookAt(position_, position_ + forward_, up_);
}

void Camera::UpdateProjection() {
  projection_ = glm::perspective(fov_, aspect_, near_, far_);
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

void Camera::SetAspect(float aspect) {
  aspect_ = aspect;
  UpdateProjection();
  UpdateFrustum();
}
