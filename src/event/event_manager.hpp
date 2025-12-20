#pragma once

#include <functional>
#include <vector>

#include <SDL3/SDL.h>

class Window;

namespace input {
class InputSystem;
}

namespace event {
class EventManager {
 public:
  using QuitCallback = std::function<void()>;

  explicit EventManager(Window& window, input::InputSystem& input);

  void PollEvents();
  void AddQuitCallback(QuitCallback callback);

  [[nodiscard]] bool ShouldQuit() const { return shouldQuit_; }

 private:
  Window& window_;
  input::InputSystem& input_;
  bool shouldQuit_ = false;
  std::vector<QuitCallback> quitCallbacks_;
};

}  // namespace event
