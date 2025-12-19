#pragma once

#include <vector>

#include "rhi/sync.hpp"
#include "rhi/texture.hpp"

namespace rhi {
/**
 * @brief Abstract swapchain interface
 */
class Swapchain {
 public:
  virtual ~Swapchain() = default;

  /**
   * @brief Present the image at imageIndex to the screen.
   *
   * @param imageIndex Index of the image to present.
   * @param waitSemaphore Semaphore to wait on before presenting.
   */
  virtual void Present(uint32_t imageIndex, Semaphore* waitSemaphore) = 0;

  /**
   * @brief Resize the swapchain to the new width and height.
   *
   * @param width New width.
   * @param height New height.
   */
  virtual void Resize(uint32_t width, uint32_t height) = 0;

  /**
   * @brief Acquire the next available image from the swapchain.
   *
   * @param signalSemaphore Semaphore to signal when the image is available.
   * @return uint32_t Index of the acquired image.
   */
  [[nodiscard]] virtual uint32_t AcquireNextImage(
      Semaphore* signalSemaphore) = 0;

  /**
   * @brief Get the images of the swapchain.
   *
   * @return const std::vector<Texture*>& Vector of swapchain images.
   */
  [[nodiscard]] virtual const std::vector<Texture*>& GetImages() const = 0;

  /**
   * @brief Get the number of images in the swapchain.
   *
   * @return uint32_t Number of images.
   */
  [[nodiscard]] virtual uint32_t GetImageCount() const = 0;

  /**
   * @brief Get the width of the swapchain images.
   *
   * @return uint32_t Width of the images.
   */
  [[nodiscard]] virtual uint32_t GetWidth() const = 0;

  /**
   * @brief Get the height of the swapchain images.
   *
   * @return uint32_t Height of the images.
   */
  [[nodiscard]] virtual uint32_t GetHeight() const = 0;
};
}  // namespace rhi
