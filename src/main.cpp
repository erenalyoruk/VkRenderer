
#include <vulkan/vulkan.hpp>

#include "gpu/instance.hpp"
#include "logger.hpp"
#include "window.hpp"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

int main() {
  quill::Backend::start();

  GetLogger()->set_log_level(
#ifndef NDEBUG
      quill::LogLevel::Debug
#else
      quill::LogLevel::Info
#endif
  );

  Window window{1280, 720, "Vulkan Window"};

  gpu::Instance instance{"Vulkan Renderer",
                         window.GetRequiredVulkanExtensions()};

  while (!window.ShouldClose()) {
    window.PollEvents();
  }

  return 0;
}
