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
   * @brief Waits for the device to become idle.
   */
  virtual void WaitIdle() = 0;

  /**
   * @brief Get the Swapchain associated with this device.
   *
   * @return Swapchain* Pointer to the Swapchain object.
   */
  [[nodiscard]] virtual Swapchain* GetSwapchain() = 0;

  /**
   * @brief Gets a queue of the specified type.
   *
   * @param type The type of queue to retrieve.
   * @return A pointer to the queue, or nullptr if not available.
   */
  [[nodiscard]] virtual Queue* GetQueue(QueueType type) = 0;
};
}  // namespace rhi
