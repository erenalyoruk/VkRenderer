#include "camera.hpp"

#include <glm/gtc/matrix_transform.hpp>

Camera::Camera(Input& input, float fovRadians, float aspect, float near,
               float far)
    : input_{input},
      fov_{fovRadians},
      aspect_{aspect},
      near_{near},
      far_{far},
      yaw_{glm::pi<float>()} {
  UpdateProjection();
  UpdateView();
  UpdateFrustum();
}

void Camera::Update(float dt) {
  // Compute forward/right/up vectors
  glm::vec3 forward{glm::cos(yaw_) * glm::cos(pitch_), glm::sin(pitch_),
                    glm::sin(yaw_) * glm::cos(pitch_)};
  glm::vec3 right =
      glm::normalize(glm::cross(forward, glm::vec3{0.0F, 1.0F, 0.0F}));
  glm::vec3 up = glm::cross(right, forward);

  // Movement
  float speed{5.0F * dt};
  if (input_.IsKeyDown(SDL_SCANCODE_W)) {
    position_ += forward * speed;
  }

  if (input_.IsKeyDown(SDL_SCANCODE_S)) {
    position_ -= forward * speed;
  }

  if (input_.IsKeyDown(SDL_SCANCODE_A)) {
    position_ -= right * speed;
  }

  if (input_.IsKeyDown(SDL_SCANCODE_D)) {
    position_ += right * speed;
  }

  if (input_.IsKeyDown(SDL_SCANCODE_SPACE)) {
    position_ += up * speed;
  }

  if (input_.IsKeyDown(SDL_SCANCODE_LSHIFT)) {
    position_ -= up * speed;
  }

  // Rotation (mouse look)
  glm::vec2 mouseDelta = input_.GetMouseDelta();
  yaw_ -= mouseDelta.x * 0.005F;
  pitch_ -= mouseDelta.y * 0.005F;
  pitch_ = glm::clamp(pitch_, (-glm::pi<float>() / 2) + 0.1F,
                      (glm::pi<float>() / 2) - 0.1F);

  UpdateView();
  UpdateFrustum();
}

void Camera::SetAspect(float aspect) {
  aspect_ = aspect;
  UpdateProjection();
  UpdateFrustum();
}

void Camera::UpdateView() {
  glm::vec3 forward{glm::sin(yaw_) * glm::cos(pitch_), glm::sin(pitch_),
                    glm::cos(yaw_) * glm::cos(pitch_)};  // Correct forward
  glm::vec3 target = position_ + forward;
  view_ = glm::lookAt(position_, target, glm::vec3{0.0F, 1.0F, 0.0F});
}

void Camera::UpdateProjection() {
  projection_ = glm::perspective(fov_, aspect_, near_, far_);
}

void Camera::UpdateFrustum() {
  glm::mat4 vp = projection_ * view_;
  frustumPlanes_[0] = vp[3] + vp[0];  // Left
  frustumPlanes_[1] = vp[3] - vp[0];  // Right
  frustumPlanes_[2] = vp[3] + vp[1];  // Bottom
  frustumPlanes_[3] = vp[3] - vp[1];  // Top
  frustumPlanes_[4] = vp[3] + vp[2];  // Near
  frustumPlanes_[5] = vp[3] - vp[2];  // Far

  for (auto& plane : frustumPlanes_) {
    float length = glm::length(glm::vec3{plane.x, plane.y, plane.z});
    plane /= length;
  }
}
