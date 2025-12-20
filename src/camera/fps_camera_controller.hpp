#pragma once

#include <SDL3/SDL.h>

#include "camera/camera_controller.hpp"
#include "input/input_system.hpp"

namespace camera {

struct FPSCameraControls {
  SDL_Scancode forward{SDL_SCANCODE_W};
  SDL_Scancode backward{SDL_SCANCODE_S};
  SDL_Scancode left{SDL_SCANCODE_A};
  SDL_Scancode right{SDL_SCANCODE_D};
  SDL_Scancode up{SDL_SCANCODE_SPACE};
  SDL_Scancode down{SDL_SCANCODE_LSHIFT};
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
