#pragma once

#include <cstdint>

#include <SDL3/SDL.h>

namespace input {
/**
 * @brief Key codes for keyboard input
 */
enum class ScanCode : uint32_t {  // NOLINT
  Unknown = 0,

  Key1 = SDL_SCANCODE_1,
  Key2 = SDL_SCANCODE_2,
  Key3 = SDL_SCANCODE_3,
  Key4 = SDL_SCANCODE_4,
  Key5 = SDL_SCANCODE_5,
  Key6 = SDL_SCANCODE_6,
  Key7 = SDL_SCANCODE_7,
  Key8 = SDL_SCANCODE_8,
  Key9 = SDL_SCANCODE_9,
  Key0 = SDL_SCANCODE_0,

  W = SDL_SCANCODE_W,
  A = SDL_SCANCODE_A,
  S = SDL_SCANCODE_S,
  D = SDL_SCANCODE_D,
  Q = SDL_SCANCODE_Q,
  E = SDL_SCANCODE_E,

  Space = SDL_SCANCODE_SPACE,
  LShift = SDL_SCANCODE_LSHIFT,
};
}  // namespace input
