target_sources(
  VkRenderer
  PRIVATE
    "main.cpp"
    "window.cpp"
    "application.cpp"
)

add_subdirectory("rhi")
add_subdirectory("backends")
add_subdirectory("platform")
add_subdirectory("input")
add_subdirectory("event")
add_subdirectory("camera")
