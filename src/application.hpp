#pragma once

#include <functional>
#include <memory>
#include <string>

#include "event/event_manager.hpp"
#include "input/input_system.hpp"
#include "platform/sdl_platform.hpp"
#include "window.hpp"

struct ApplicationConfig {
  WindowConfig window;
  bool enableInput{true};
};

class Application {
 public:
  using UpdateCallback = std::function<void(float)>;
  using RenderCallback = std::function<void(float)>;

  explicit Application(const ApplicationConfig& config);
  Application(int width, int height, const std::string& title);
  ~Application();

  void Run(const UpdateCallback& update = nullptr,
           const RenderCallback& render = nullptr);

  [[nodiscard]] Window& GetWindow() { return *window_; }
  [[nodiscard]] input::InputSystem& GetInput() { return *input_; }
  [[nodiscard]] event::EventManager& GetEventManager() { return *events_; }

  void RequestQuit() { shouldQuit_ = true; }

 private:
  std::unique_ptr<platform::SDLPlatform> platform_;
  std::unique_ptr<Window> window_;
  std::unique_ptr<input::InputSystem> input_;
  std::unique_ptr<event::EventManager> events_;
  bool shouldQuit_{false};
};
