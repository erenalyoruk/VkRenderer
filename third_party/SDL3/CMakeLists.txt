set(SDL_STATIC OFF CACHE BOOL "Build SDL as a static library")
set(SDL_SHARED ON CACHE BOOL "Build SDL as a shared library")
set(SDL_TESTS OFF CACHE BOOL "Build SDL test programs")

FetchContent_Declare(
  SDL3
  GIT_REPOSITORY  https://github.com/libsdl-org/SDL.git
  GIT_TAG         release-3.2.26
  GIT_SHALLOW     TRUE
  SYSTEM
  EXCLUDE_FROM_ALL
)

FetchContent_MakeAvailable(SDL3)
