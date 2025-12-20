#pragma once

#include <bitset>

#include <SDL3/SDL.h>
#include <glm/vec2.hpp>

namespace input {
enum class MouseButton : uint8_t {
  Left = 0,
  Middle = 1,
  Right = 2,
  X1 = 3,
  X2 = 4
};

class InputSystem {
 public:
  void BeginFrame();
  void ProcessEvent(const SDL_Event& event);

  // Keyboard
  [[nodiscard]] bool IsKeyDown(SDL_Scancode key) const;
  [[nodiscard]] bool IsKeyPressed(SDL_Scancode key) const;
  [[nodiscard]] bool IsKeyReleased(SDL_Scancode key) const;

  // Mouse
  [[nodiscard]] bool IsMouseDown(MouseButton button) const;
  [[nodiscard]] bool IsMousePressed(MouseButton button) const;
  [[nodiscard]] bool IsMouseReleased(MouseButton button) const;
  [[nodiscard]] glm::vec2 GetMousePosition() const { return mousePosition_; }
  [[nodiscard]] glm::vec2 GetMouseDelta() const { return mouseDelta_; }
  [[nodiscard]] float GetMouseWheel() const { return mouseWheel_; }

 private:
  std::bitset<SDL_SCANCODE_COUNT> currentKeys_;
  std::bitset<SDL_SCANCODE_COUNT> previousKeys_;

  std::bitset<5> currentMouse_;
  std::bitset<5> previousMouse_;

  glm::vec2 mousePosition_{0.0F};
  glm::vec2 mouseDelta_{0.0F};
  float mouseWheel_{0.0F};
};
}  // namespace input
