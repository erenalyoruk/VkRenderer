#include "application.hpp"

#include <cstdint>

#include <SDL3/SDL.h>

Application::Application(int width, int height, const std::string& title)
    : window_{width, height, title},
      camera_{
          window_.GetInput(),
          glm::radians(60.0F),
          static_cast<float>(width) / static_cast<float>(height),
          0.1F,
          100.0F,
      } {}

void Application::Run(const std::function<void()>& updateFn) {
  float deltaTime{0.0F};
  uint64_t lastTime{SDL_GetPerformanceCounter()};

  while (!window_.ShouldClose()) {
    const uint64_t now{SDL_GetPerformanceCounter()};
    deltaTime = static_cast<float>(now - lastTime) /
                static_cast<float>(SDL_GetPerformanceFrequency());
    lastTime = now;

    window_.PollEvents();
    camera_.Update(deltaTime);
    updateFn();
  }
}
