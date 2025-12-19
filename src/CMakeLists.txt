target_sources(
  VkRenderer
  PRIVATE
    "main.cpp"
    "window.cpp"
    "input.cpp"
    "application.cpp"
)

add_subdirectory("gpu")
