#pragma once

#include <SDL3/SDL.h>

#include "camera/camera_controller.hpp"
#include "input/input_system.hpp"

namespace camera {

struct FPSCameraControls {
  input::ScanCode forward{input::ScanCode::W};
  input::ScanCode backward{input::ScanCode::S};
  input::ScanCode left{input::ScanCode::A};
  input::ScanCode right{input::ScanCode::D};
  input::ScanCode up{input::ScanCode::Space};
  input::ScanCode down{input::ScanCode::LShift};
  input::MouseButton rotateButton{input::MouseButton::Right};
};

class FPSCameraController {
 public:
  FPSCameraController(Camera& camera, const FPSCameraControls& controls = {});

  void Update(const input::InputSystem& input, float deltaTime);

  void SetMovementSpeed(float speed) { movementSpeed_ = speed; }
  void SetMouseSensitivity(float sensitivity) {
    mouseSensitivity_ = sensitivity;
  }

 private:
  Camera& camera_;
  FPSCameraControls controls_;
  float movementSpeed_{5.0F};
  float mouseSensitivity_{0.1F};
};

}  // namespace camera
