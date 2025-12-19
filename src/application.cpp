#include "application.hpp"

Application::Application(int width, int height, const std::string& title)
    : window_{width, height, title} {}

void Application::Run(const std::function<void()>& updateFn) {
  while (!window_.ShouldClose()) {
    window_.PollEvents();
    updateFn();
  }
}
