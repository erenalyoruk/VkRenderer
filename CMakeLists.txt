cmake_minimum_required(VERSION 3.28)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(
  VkRenderer
    VERSION 0.1.0
    LANGUAGES C CXX
)

add_executable(VkRenderer)

target_link_libraries(
  VkRenderer
  PRIVATE
    quill::quill
    SDL3::SDL3
    Vulkan::Headers
    ImGui::ImGui
    glm::glm
    GPUOpen::VulkanMemoryAllocator
)

add_subdirectory("src")
add_subdirectory("third_party")
