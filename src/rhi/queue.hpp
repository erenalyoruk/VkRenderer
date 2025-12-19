#pragma once

#include <cstdint>
#include <span>

#include "rhi/command.hpp"
#include "rhi/swapchain.hpp"
#include "rhi/sync.hpp"

namespace rhi {
/**
 * @brief Types of command queues.
 */
enum class QueueType : uint8_t {
  Graphics,  // Includes graphics and compute capabilities
  Compute,   // Compute-only queue
  Transfer,  // Transfer-only queue
};

/**
 * @brief Abstract base class for command queues in the RHI.
 */
class Queue {
 public:
  virtual ~Queue() = default;

  /**
   * @brief Submits command buffers to the queue.
   *
   * @param commandBuffers The command buffers to submit.
   * @param waitSemaphores Semaphores to wait on before execution.
   * @param signalSemaphores Semaphores to signal after execution.
   * @param fence Optional fence to signal when submission completes.
   */
  virtual void Submit(std::span<CommandBuffer* const> commandBuffers,
                      std::span<Semaphore* const> waitSemaphores,
                      std::span<Semaphore* const> signalSemaphores,
                      Fence* fence = nullptr) = 0;

  /**
   * @brief Presents the swapchain using this queue.
   *
   * @param swapchain The swapchain to present.
   * @param imageIndex The index of the image to present.
   * @param waitSemaphores Semaphores to wait on before presenting.
   */
  virtual void Present(Swapchain* swapchain, uint32_t imageIndex,
                       std::span<Semaphore* const> waitSemaphores) = 0;

  /**
   * @brief Gets the type of the queue.
   *
   * @return The queue type.
   */
  [[nodiscard]] virtual QueueType GetType() const = 0;
};
}  // namespace rhi
