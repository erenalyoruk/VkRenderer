FetchContent_Declare(
  imgui
  GIT_REPOSITORY  https://github.com/ocornut/imgui.git
  GIT_TAG         v1.92.5
  GIT_SHALLOW     TRUE
  SYSTEM
  EXCLUDE_FROM_ALL
)

FetchContent_MakeAvailable(imgui)

add_library(
  ImGui
  "${imgui_SOURCE_DIR}/imgui.cpp"
  "${imgui_SOURCE_DIR}/imgui_draw.cpp"
  "${imgui_SOURCE_DIR}/imgui_tables.cpp"
  "${imgui_SOURCE_DIR}/imgui_widgets.cpp"
  "${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp"
  "${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp"
)
add_library(ImGui::ImGui ALIAS ImGui)

target_include_directories(
  ImGui
  PUBLIC
    "${imgui_SOURCE_DIR}"
    "${imgui_SOURCE_DIR}/backends"
)

target_link_libraries(
  ImGui
  PUBLIC
    SDL3::SDL3
    Vulkan::Headers
)

target_compile_definitions(ImGui PUBLIC IMGUI_IMPL_VULKAN_NO_PROTOTYPES)
