target_sources(
  VkRenderer
  PRIVATE
    "main.cpp"
    "window.cpp"
    "input.cpp"
    "application.cpp"
    "camera.cpp"
)

add_subdirectory("vulkan")
add_subdirectory("rendering")
add_subdirectory("rhi")
add_subdirectory("backends")
