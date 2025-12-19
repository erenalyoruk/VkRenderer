#pragma once

#include "rhi/queue.hpp"
#include "rhi/swapchain.hpp"

namespace rhi {
/**
 * @brief Abstract representation of a rendering device.
 */
class Device {
 public:
  virtual ~Device() = default;

  /**
   * @brief Get the Swapchain associated with this device.
   *
   * @return Swapchain* Pointer to the Swapchain object.
   */
  virtual Swapchain* GetSwapchain() = 0;

  /**
   * @brief Gets a queue of the specified type.
   *
   * @param type The type of queue to retrieve.
   * @return A pointer to the queue, or nullptr if not available.
   */
  virtual Queue* GetQueue(QueueType type) = 0;
};
}  // namespace rhi
