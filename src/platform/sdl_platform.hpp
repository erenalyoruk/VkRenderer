#pragma once

#include <memory>

namespace platform {
class SDLPlatform {
 public:
  SDLPlatform();
  ~SDLPlatform();

  SDLPlatform(const SDLPlatform&) = delete;
  SDLPlatform& operator=(const SDLPlatform&) = delete;
  SDLPlatform(SDLPlatform&&) noexcept = delete;
  SDLPlatform& operator=(SDLPlatform&&) noexcept = delete;

 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};
}  // namespace platform
