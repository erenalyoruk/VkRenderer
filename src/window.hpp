#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.hpp>

struct WindowConfig {
  int width{1280};
  int height{720};
  std::string title{"VkRenderer"};
  bool resizable{true};
  bool highDPI{true};
  bool vulkanSupport{true};
};

class Window {
 public:
  using ResizeCallback = std::function<void(int, int)>;

  explicit Window(const WindowConfig& config);
  ~Window();

  Window(const Window&) = delete;
  Window& operator=(const Window&) = delete;
  Window(Window&&) noexcept;
  Window& operator=(Window&&) noexcept;

  void AddResizeCallback(ResizeCallback callback);

  [[nodiscard]] auto GetRequiredVulkanExtensions() const
      -> std::vector<const char*>;
  [[nodiscard]] vk::SurfaceKHR CreateSurface(vk::Instance instance) const;

  [[nodiscard]] SDL_Window* GetHandle() const { return window_.get(); }
  [[nodiscard]] int GetWidth() const { return width_; }
  [[nodiscard]] int GetHeight() const { return height_; }
  [[nodiscard]] float GetAspectRatio() const {
    return static_cast<float>(width_) / static_cast<float>(height_);
  }

  // Internal use by event system
  void NotifyResize(int width, int height);

 private:
  std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> window_;
  int width_;
  int height_;
  std::vector<ResizeCallback> resizeCallbacks_;
};
