#pragma once

#include <memory>
#include <string>
#include <vector>

#include <SDL3/SDL.h>
#include <SDl3/SDL_vulkan.h>
#include <vulkan/vulkan.hpp>

class Window {
 public:
  Window(int width, int height, const std::string& title);
  ~Window();

  Window(const Window&) = delete;
  Window& operator=(const Window&) = delete;

  Window(Window&&) noexcept;
  Window& operator=(Window&&) noexcept;

  void PollEvents();

  [[nodiscard]] auto GetRequiredVulkanExtensions() const
      -> std::vector<const char*>;

  [[nodiscard]] bool CreateSurface(VkInstance instance,
                                   VkSurfaceKHR* surface) const;

  [[nodiscard]] SDL_Window& GetHandle() const { return *window_; }

  [[nodiscard]] bool ShouldClose() const { return shouldClose_; }

 private:
  std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> window_{
      nullptr, SDL_DestroyWindow};

  int width_{0};
  int height_{0};

  bool shouldClose_{false};
};
