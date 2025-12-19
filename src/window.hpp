#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <SDL3/SDL.h>
#include <SDl3/SDL_vulkan.h>
#include <vulkan/vulkan.hpp>

#include "input.hpp"

class Window {
 public:
  Window(int width, int height, const std::string& title);
  ~Window();

  Window(const Window&) = delete;
  Window& operator=(const Window&) = delete;

  Window(Window&&) noexcept;
  Window& operator=(Window&&) noexcept;

  void PollEvents();

  void AddOnResize(std::function<void(int, int)> callback) {
    resizeCallback_.push_back(std::move(callback));
  }

  [[nodiscard]] auto GetRequiredVulkanExtensions() const
      -> std::vector<const char*>;

  [[nodiscard]] vk::SurfaceKHR CreateSurface(vk::Instance instance) const;

  [[nodiscard]] SDL_Window& GetHandle() const { return *window_; }

  [[nodiscard]] bool ShouldClose() const { return shouldClose_; }

  [[nodiscard]] Input& GetInput() { return input_; }

  [[nodiscard]] int GetWidth() const { return width_; }
  [[nodiscard]] int GetHeight() const { return height_; }

 private:
  std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> window_{
      nullptr, SDL_DestroyWindow};

  int width_{0};
  int height_{0};

  bool shouldClose_{false};

  std::vector<std::function<void(int, int)>> resizeCallback_;

  Input input_;
};
