#pragma once

#include <functional>
#include <string>

#include "camera.hpp"
#include "input.hpp"
#include "window.hpp"

class Application {
 public:
  Application(int width, int height, const std::string& title);

  void Run(const std::function<void()>& updateFn = []() {});

  [[nodiscard]] Window& GetWindow() { return window_; }

  [[nodiscard]] Input& GetInput() { return window_.GetInput(); }

  [[nodiscard]] Camera& GetCamera() { return camera_; }

 private:
  Window window_;
  Camera camera_;
};
