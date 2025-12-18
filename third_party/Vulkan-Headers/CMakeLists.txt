FetchContent_Declare(
  Vulkan-Headers
  GIT_REPOSITORY  https://github.com/KhronosGroup/Vulkan-Headers.git
  GIT_TAG         vulkan-sdk-1.4.328.1
  GIT_SHALLOW     TRUE
  SYSTEM
  EXCLUDE_FROM_ALL
)

FetchContent_MakeAvailable(Vulkan-Headers)
