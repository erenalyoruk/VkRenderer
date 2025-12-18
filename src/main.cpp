#include <vulkan/vulkan.hpp>

#include "window.hpp"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

int main() {
  Window window{1280, 720, "Vulkan Window"};

  while (!window.ShouldClose()) {
    window.PollEvents();
  }

  return 0;
}
