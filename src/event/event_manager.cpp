#include "event/event_manager.hpp"

#include <utility>

#include "input/input_system.hpp"
#include "window.hpp"

namespace event {
EventManager::EventManager(Window& window, input::InputSystem& input)
    : window_{window}, input_{input} {}

void EventManager::PollEvents() {
  input_.BeginFrame();

  SDL_Event event{};
  while (SDL_PollEvent(&event)) {
    input_.ProcessEvent(event);

    switch (event.type) {
      case SDL_EVENT_QUIT:
      case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        shouldQuit_ = true;
        for (const auto& callback : quitCallbacks_) {
          callback();
        }
        break;

      case SDL_EVENT_WINDOW_RESIZED:
        window_.NotifyResize(event.window.data1, event.window.data2);
        break;

      default:
        break;
    }
  }
}

void EventManager::AddQuitCallback(QuitCallback callback) {
  quitCallbacks_.push_back(std::move(callback));
}

}  // namespace event
