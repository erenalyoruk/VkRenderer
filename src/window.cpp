#include "window.hpp"

#include <cassert>
#include <span>
#include <stdexcept>
#include <vector>

#include "logger.hpp"

namespace {
void DestroySDLWindow(SDL_Window* window) {
  if (window != nullptr) {
    LOG_DEBUG("Destroying SDL window.");
    SDL_DestroyWindow(window);
  }
}
}  // namespace

Window::Window(int width, int height, const std::string& title)
    : window_{nullptr, DestroySDLWindow}, width_{width}, height_{height} {
  if (!SDL_Init(SDL_INIT_VIDEO)) {
    LOG_CRITICAL("Failed to initialize SDL! SDL_Error: {}", SDL_GetError());
    throw std::runtime_error{"Failed to initialize SDL."};
  }

  SDL_WindowFlags windowFlags{SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE |
                              SDL_WINDOW_HIGH_PIXEL_DENSITY};

  window_.reset(SDL_CreateWindow(title.c_str(), width_, height_, windowFlags));
  if (window_ == nullptr) {
    LOG_CRITICAL("Failed to create SDL window! SDL_Error: {}", SDL_GetError());
    throw std::runtime_error{"Failed to create SDL window."};
  }

  LOG_DEBUG("SDL window created.");
}

Window::~Window() {
  window_.reset();

  SDL_Quit();
  LOG_DEBUG("SDL terminated.");
}

Window::Window(Window&& other) noexcept
    : window_{std::exchange(other.window_, nullptr)},
      width_{std::exchange(other.width_, 0)},
      height_{std::exchange(other.height_, 0)},
      shouldClose_{std::exchange(other.shouldClose_, false)} {}

Window& Window::operator=(Window&& other) noexcept {
  if (this == &other) {
    return *this;
  }

  window_ = std::exchange(other.window_, nullptr);
  width_ = std::exchange(other.width_, 0);
  height_ = std::exchange(other.height_, 0);
  shouldClose_ = std::exchange(other.shouldClose_, false);

  return *this;
}

void Window::PollEvents() {
  SDL_Event event{};
  while (SDL_PollEvent(&event)) {
    input_.ProcessEvent(event);

    switch (event.type) {
      case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        shouldClose_ = true;
        break;
      case SDL_EVENT_WINDOW_RESIZED: {
        width_ = event.window.data1;
        height_ = event.window.data2;
        if (resizeCallback_ != nullptr) {
          resizeCallback_(width_, height_);
        }
        break;
      }
      default:
        break;
    }
  }
}

std::vector<const char*> Window::GetRequiredVulkanExtensions() const {
  ((void)this);

  uint32_t count{0};
  const char* const* extensions{SDL_Vulkan_GetInstanceExtensions(&count)};

  if (extensions == nullptr) {
    throw std::runtime_error{
        "Failed to get required Vulkan extensions from SDL"};
  }

  std::span<const char* const> span{extensions, count};
  return {span.begin(), span.end()};
}

vk::SurfaceKHR Window::CreateSurface(vk::Instance instance) const {
  VkSurfaceKHR surface{};
  SDL_Vulkan_CreateSurface(window_.get(), instance, nullptr, &surface);

  return vk::SurfaceKHR{surface};
}
