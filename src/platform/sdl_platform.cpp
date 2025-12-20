#include "platform/sdl_platform.hpp"

#include <stdexcept>

#include <SDL3/SDL.h>

#include "logger.hpp"

namespace platform {

class SDLPlatform::Impl {
 public:
  Impl() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
      LOG_CRITICAL("Failed to initialize SDL! SDL_Error: {}", SDL_GetError());
      throw std::runtime_error{"Failed to initialize SDL."};
    }
    LOG_DEBUG("SDL initialized.");
  }

  ~Impl() {
    SDL_Quit();
    LOG_DEBUG("SDL terminated.");
  }

  Impl(const Impl&) = delete;
  Impl& operator=(const Impl&) = delete;
  Impl(Impl&&) noexcept = delete;
  Impl& operator=(Impl&&) noexcept = delete;
};

SDLPlatform::SDLPlatform() : impl_{std::make_unique<Impl>()} {}

SDLPlatform::~SDLPlatform() = default;

}  // namespace platform
