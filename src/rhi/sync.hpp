#pragma once

#include <cstdint>

namespace rhi {
/**
 * @brief A synchronization primitive that can be used to coordinate CPU and GPU
 * operations.
 */
class Fence {
 public:
  virtual ~Fence() = default;

  /**
   * @brief Waits until the fence is signaled or the timeout expires.
   *
   * @param timeout The maximum time to wait in nanoseconds. Default is
   * UINT64_MAX (infinite wait).
   */
  virtual void Wait(uint64_t timeout = UINT64_MAX) = 0;

  /**
   * @brief Resets the fence to the unsignaled state.
   */
  virtual void Reset() = 0;

  /**
   * @brief Checks if the fence is signaled.
   *
   * @return true if the fence is signaled, otherwise false.
   * @return false if the fence is not signaled.
   */
  [[nodiscard]] virtual bool IsSignaled() const = 0;
};

/**
 * @brief A synchronization primitive that can be used to coordinate GPU
 * operations.
 */
class Semaphore {
 public:
  virtual ~Semaphore() = default;
};
}  // namespace rhi
