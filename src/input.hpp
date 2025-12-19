#pragma once

#include <array>

#include <SDL3/SDL.h>
#include <glm/vec2.hpp>

class Input {
 public:
  void Update();
  void ProcessEvent(const SDL_Event& event);

  [[nodiscard]] bool IsKeyDown(SDL_Scancode scancode) const {
    return currentKeys_.at(scancode);
  }

  [[nodiscard]] bool IsKeyPressed(SDL_Scancode scancode) const {
    return currentKeys_.at(scancode) && !previousKeys_.at(scancode);
  }

  [[nodiscard]] bool IsKeyReleased(SDL_Scancode scancode) const {
    return !currentKeys_.at(scancode) && previousKeys_.at(scancode);
  }

  [[nodiscard]] bool IsMouseButtonDown(SDL_MouseButtonFlags button) const {
    return currentMouseButtons_.at(button - 1);
  }

  [[nodiscard]] bool IsMouseButtonPressed(SDL_MouseButtonFlags button) const {
    return currentMouseButtons_.at(button - 1) &&
           !previousMouseButtons_.at(button - 1);
  }

  [[nodiscard]] bool IsMouseButtonReleased(SDL_MouseButtonFlags button) const {
    return !currentMouseButtons_.at(button - 1) &&
           previousMouseButtons_.at(button - 1);
  }

  [[nodiscard]] glm::vec2 GetMousePosition() const { return mousePosition_; }
  [[nodiscard]] glm::vec2 GetMouseDelta() const { return mouseDelta_; }
  [[nodiscard]] float GetMouseScroll() const { return mouseWheelDelta_; }

 private:
  std::array<bool, SDL_SCANCODE_COUNT> currentKeys_{false};
  std::array<bool, SDL_SCANCODE_COUNT> previousKeys_{false};

  std::array<bool, 6> currentMouseButtons_{false};
  std::array<bool, 6> previousMouseButtons_{false};
  glm::vec2 mousePosition_{0.0F, 0.0F};
  glm::vec2 mouseDelta_{0.0F, 0.0F};
  float mouseWheelDelta_{0.0F};
};
