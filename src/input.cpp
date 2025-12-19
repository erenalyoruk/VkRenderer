#include "input.hpp"

void Input::Update() {
  previousKeys_ = currentKeys_;
  previousMouseButtons_ = currentMouseButtons_;

  mouseDelta_ = glm::vec2(0.0F, 0.0F);
  mouseWheelDelta_ = 0.0F;
}

void Input::ProcessEvent(const SDL_Event& event) {
  switch (event.type) {
    case SDL_EVENT_KEY_DOWN:
      currentKeys_.at(event.key.scancode) = true;
      break;
    case SDL_EVENT_KEY_UP:
      currentKeys_.at(event.key.scancode) = false;
      break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
      currentMouseButtons_.at(event.button.button - 1) = true;
      break;
    case SDL_EVENT_MOUSE_BUTTON_UP:
      currentMouseButtons_.at(event.button.button - 1) = false;
      break;
    case SDL_EVENT_MOUSE_MOTION: {
      glm::vec2 mouseDelta{
          static_cast<float>(event.motion.xrel),
          static_cast<float>(event.motion.yrel),
      };
      mouseDelta_ += mouseDelta;

      mousePosition_ = glm::vec2{
          static_cast<float>(event.motion.x),
          static_cast<float>(event.motion.y),
      };
      break;
    }
    case SDL_EVENT_MOUSE_WHEEL:
      mouseWheelDelta_ += static_cast<float>(event.wheel.y);
      break;
    default:
      break;
  }
}
