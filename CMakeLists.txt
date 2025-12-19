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

target_compile_definitions(
  VkRenderer
  PUBLIC
    VULKAN_HPP_DISPATCH_LOADER_DYNAMIC
    VULKAN_HPP_NO_CONSTRUCTORS
)

target_link_libraries(
  VkRenderer
  PRIVATE
    quill::quill
    SDL3::SDL3
    Vulkan::Headers
    ImGui::ImGui
    glm::glm
    GPUOpen::VulkanMemoryAllocator
    tinygltf
    EnTT::EnTT
)

target_include_directories(
  VkRenderer
  PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

add_subdirectory("src")
add_subdirectory("third_party")

if (WIN32)
  add_custom_command (
    TARGET VkRenderer POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy -t "$<TARGET_FILE_DIR:VkRenderer>"
            "$<TARGET_RUNTIME_DLLS:VkRenderer>"
    USES_TERMINAL
    COMMAND_EXPAND_LISTS
  )
endif ()

find_program(
  GLSLC_EXECUTABLE
  NAMES "glslc"
  HINTS
    "$ENV{VULKAN_SDK}/Bin"
    "$ENV{VULKAN_SDK}/bin"
)

if (NOT GLSLC_EXECUTABLE)
  message(FATAL_ERROR "glslc not found! Please install the Vulkan SDK.")
endif()

set(SHADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders")
set(SPIRV_DIR "${CMAKE_CURRENT_BINARY_DIR}/assets/shaders")

file(GLOB SHADERS "${SHADER_DIR}/*.vert" "${SHADER_DIR}/*.frag" "${SHADER_DIR}/*.comp")

set(SPIRV_OUTPUTS "")

foreach (SHADER ${SHADERS})
  get_filename_component(SHADER_NAME ${SHADER} NAME)
  set(SPIRV "${SPIRV_DIR}/${SHADER_NAME}.spv")

  add_custom_command(
    OUTPUT ${SPIRV}
    COMMAND ${GLSLC_EXECUTABLE} -o ${SPIRV} ${SHADER}
    DEPENDS ${SHADER}
    COMMENT "Compiling shader ${SHADER_NAME}"
    VERBATIM
  )
  list(APPEND SPIRV_OUTPUTS ${SPIRV})
endforeach()

add_custom_target(compile_shaders ALL DEPENDS ${SPIRV_OUTPUTS})
add_dependencies(VkRenderer compile_shaders)

add_custom_command(
  TARGET VkRenderer POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          "${SPIRV_DIR}"
          "$<TARGET_FILE_DIR:VkRenderer>/assets/shaders"
)

add_custom_command(
  TARGET VkRenderer POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          "${CMAKE_CURRENT_SOURCE_DIR}/assets/models"
          "$<TARGET_FILE_DIR:VkRenderer>/assets/models"
)
