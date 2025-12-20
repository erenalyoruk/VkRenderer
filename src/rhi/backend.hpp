#pragma once

#include <cstdint>
#include <memory>

#include "rhi/device.hpp"
#include "rhi/factory.hpp"
#include "window.hpp"

namespace rhi {
/**
 * @brief Supported RHI Backend Types
 */
enum class BackendType : uint8_t {
  Vulkan,
  DX12,
};

/**
 * @brief Represents an RHI Backend with its Device and Factory
 */
struct Backend {
  // The RHI Device
  std::unique_ptr<Device> device;

  // The RHI Factory
  std::unique_ptr<Factory> factory;
};

/**
 * @brief Factory for creating RHI backend factories
 */
class BackendFactory {
 public:
  /**
   * @brief Create a Factory for the specified backend type
   *
   * @param type The backend type
   * @param window The window to create the backend for
   * @param enableValidation Whether to enable validation layers (if supported)
   * @return Backend The created backend with its device and factory
   */
  static Backend Create(BackendType type, Window& window, uint32_t width,
                        uint32_t height, bool enableValidation = false);
};
}  // namespace rhi
