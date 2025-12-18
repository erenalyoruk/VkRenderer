
#include <vulkan/vulkan.hpp>

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

  while (!window.ShouldClose()) {
    window.PollEvents();
  }

  return 0;
}
