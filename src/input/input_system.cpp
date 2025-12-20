#include "input_system.hpp"

namespace input {

void InputSystem::BeginFrame() {
  previousKeys_ = currentKeys_;
  previousMouse_ = currentMouse_;
  mouseDelta_ = glm::vec2{0.0F};
  mouseWheel_ = 0.0F;
}

void InputSystem::ProcessEvent(const SDL_Event& event) {
  switch (event.type) {
    case SDL_EVENT_KEY_DOWN:
      if (event.key.scancode < SDL_SCANCODE_COUNT) {
        currentKeys_[event.key.scancode] = true;
      }
      break;

    case SDL_EVENT_KEY_UP:
      if (event.key.scancode < SDL_SCANCODE_COUNT) {
        currentKeys_[event.key.scancode] = false;
      }
      break;

    case SDL_EVENT_MOUSE_BUTTON_DOWN:
      if (event.button.button > 0 && event.button.button <= 5) {
        currentMouse_[event.button.button - 1] = true;
      }
      break;

    case SDL_EVENT_MOUSE_BUTTON_UP:
      if (event.button.button > 0 && event.button.button <= 5) {
        currentMouse_[event.button.button - 1] = false;
      }
      break;

    case SDL_EVENT_MOUSE_MOTION:
      mouseDelta_ += glm::vec2{static_cast<float>(event.motion.xrel),
                               static_cast<float>(event.motion.yrel)};
      mousePosition_ = glm::vec2{static_cast<float>(event.motion.x),
                                 static_cast<float>(event.motion.y)};
      break;

    case SDL_EVENT_MOUSE_WHEEL:
      mouseWheel_ += static_cast<float>(event.wheel.y);
      break;

    default:
      break;
  }
}

bool InputSystem::IsKeyDown(ScanCode key) const {
  return static_cast<SDL_Scancode>(key) < SDL_SCANCODE_COUNT &&
         currentKeys_[static_cast<SDL_Scancode>(key)];
}

bool InputSystem::IsKeyPressed(ScanCode key) const {
  return static_cast<SDL_Scancode>(key) < SDL_SCANCODE_COUNT &&
         currentKeys_[static_cast<SDL_Scancode>(key)] &&
         !previousKeys_[static_cast<SDL_Scancode>(key)];
}

bool InputSystem::IsKeyReleased(ScanCode key) const {
  return static_cast<SDL_Scancode>(key) < SDL_SCANCODE_COUNT &&
         !currentKeys_[static_cast<SDL_Scancode>(key)] &&
         previousKeys_[static_cast<SDL_Scancode>(key)];
}

bool InputSystem::IsMouseDown(MouseButton button) const {
  auto idx = static_cast<size_t>(button);
  return idx < 5 && currentMouse_[idx];
}

bool InputSystem::IsMousePressed(MouseButton button) const {
  auto idx = static_cast<size_t>(button);
  return idx < 5 && currentMouse_[idx] && !previousMouse_[idx];
}

bool InputSystem::IsMouseReleased(MouseButton button) const {
  auto idx = static_cast<size_t>(button);
  return idx < 5 && !currentMouse_[idx] && previousMouse_[idx];
}
}  // namespace input
