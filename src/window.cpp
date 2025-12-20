#include "window.hpp"

#include <span>
#include <stdexcept>

#include "logger.hpp"

Window::Window(const WindowConfig& config)
    : window_{nullptr, SDL_DestroyWindow},
      width_{config.width},
      height_{config.height} {
  SDL_WindowFlags flags = 0;
  if (config.vulkanSupport) {
    flags |= SDL_WINDOW_VULKAN;
  }
  if (config.resizable) {
    flags |= SDL_WINDOW_RESIZABLE;
  }
  if (config.highDPI) {
    flags |= SDL_WINDOW_HIGH_PIXEL_DENSITY;
  }

  window_.reset(SDL_CreateWindow(config.title.c_str(), width_, height_, flags));
  if (window_ == nullptr) {
    LOG_CRITICAL("Failed to create SDL window! SDL_Error: {}", SDL_GetError());
    throw std::runtime_error{"Failed to create SDL window."};
  }

  LOG_DEBUG("SDL window created.");
}

Window::~Window() { window_.reset(); }

Window::Window(Window&& other) noexcept
    : window_{std::move(other.window_)},
      width_{std::exchange(other.width_, 0)},
      height_{std::exchange(other.height_, 0)},
      resizeCallbacks_{std::move(other.resizeCallbacks_)} {}

Window& Window::operator=(Window&& other) noexcept {
  if (this != &other) {
    window_ = std::move(other.window_);
    width_ = std::exchange(other.width_, 0);
    height_ = std::exchange(other.height_, 0);
    resizeCallbacks_ = std::move(other.resizeCallbacks_);
  }
  return *this;
}

void Window::AddResizeCallback(ResizeCallback callback) {
  resizeCallbacks_.push_back(std::move(callback));
}

std::vector<const char*> Window::GetRequiredVulkanExtensions() const {
  (void)this;

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

void Window::NotifyResize(int width, int height) {
  width_ = width;
  height_ = height;
  for (const auto& callback : resizeCallbacks_) {
    callback(width, height);
  }
}
