#include "camera/fps_camera_controller.hpp"

namespace camera {
FPSCameraController::FPSCameraController(camera::Camera& camera,
                                         const FPSCameraControls& controls)
    : camera_{camera}, controls_{controls} {}

void FPSCameraController::Update(const input::InputSystem& input,
                                 float deltaTime) {
  // Handle Mouse Rotation
  if (input.IsMouseDown(controls_.rotateButton)) {
    glm::vec2 mouseDelta = input.GetMouseDelta();
    float yaw =
        camera_.GetYaw() + glm::radians(mouseDelta.x * mouseSensitivity_);
    float pitch =
        camera_.GetPitch() - glm::radians(mouseDelta.y * mouseSensitivity_);
    camera_.SetRotation(yaw, pitch);
  }

  // Handle Movement
  glm::vec3 position = camera_.GetPosition();
  float speed = movementSpeed_ * deltaTime;

  if (input.IsKeyDown(controls_.forward)) {
    position += camera_.GetForward() * speed;
  }
  if (input.IsKeyDown(controls_.backward)) {
    position -= camera_.GetForward() * speed;
  }
  if (input.IsKeyDown(controls_.left)) {
    position -= camera_.GetRight() * speed;
  }
  if (input.IsKeyDown(controls_.right)) {
    position += camera_.GetRight() * speed;
  }

  constexpr glm::vec3 kWorldUp{0.0F, 1.0F, 0.0F};
  if (input.IsKeyDown(controls_.up)) {
    position += kWorldUp * speed;
  }
  if (input.IsKeyDown(controls_.down)) {
    position -= kWorldUp * speed;
  }

  camera_.SetPosition(position);
}
}  // namespace camera
