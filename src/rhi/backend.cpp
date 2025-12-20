#include "rhi/backend.hpp"

#include <stdexcept>

#include "backends/vulkan/vulkan_context.hpp"
#include "backends/vulkan/vulkan_factory.hpp"

namespace rhi {
Backend BackendFactory::Create(BackendType type, Window& window, uint32_t width,
                               uint32_t height, bool enableValidation) {
  switch (type) {
    case BackendType::Vulkan: {
      auto context = std::make_unique<backends::vulkan::VulkanContext>(
          window, width, height, enableValidation);
      auto factory =
          std::make_unique<backends::vulkan::VulkanFactory>(*context);
      return {.device = std::move(context), .factory = std::move(factory)};
    }
    default:
      throw std::runtime_error("Unknown backend");
  }
}
}  // namespace rhi
