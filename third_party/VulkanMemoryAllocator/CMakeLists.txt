set(VMA_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
  VulkanMemoryAllocator
  GIT_REPOSITORY          https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
  GIT_TAG                 v3.3.0
  GIT_SHALLOW             TRUE
  SYSTEM
  EXCLUDE_FROM_ALL
)

FetchContent_MakeAvailable(VulkanMemoryAllocator)
