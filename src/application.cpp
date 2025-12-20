#include "application.hpp"

#include <SDL3/SDL.h>

Application::Application(const ApplicationConfig& config)
    : platform_{std::make_unique<platform::SDLPlatform>()},
      window_{std::make_unique<Window>(config.window)},
      input_{std::make_unique<input::InputSystem>()},
      events_{std::make_unique<event::EventManager>(*window_, *input_)} {}

Application::Application(int width, int height, const std::string& title)
    : Application(ApplicationConfig{.window = WindowConfig{
                                        .width = width,
                                        .height = height,
                                        .title = title,
                                    }}) {}

Application::~Application() = default;

void Application::Run(const UpdateCallback& update,
                      const RenderCallback& render) {
  float deltaTime{0.0F};
  uint64_t lastTime{SDL_GetPerformanceCounter()};

  while (!events_->ShouldQuit() && !shouldQuit_) {
    const uint64_t now{SDL_GetPerformanceCounter()};
    deltaTime = static_cast<float>(now - lastTime) /
                static_cast<float>(SDL_GetPerformanceFrequency());
    lastTime = now;

    events_->PollEvents();

    if (update) {
      update(deltaTime);
    }

    if (render) {
      render();
    }
  }
}
