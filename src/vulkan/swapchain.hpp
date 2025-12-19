#pragma once

#include <cstdint>
#include <vector>

#include <vulkan/vulkan.hpp>

#include "vulkan/context.hpp"

namespace vulkan {
class Swapchain {
 public:
  Swapchain(Context& context, uint32_t width, uint32_t height);
  ~Swapchain() = default;

  Swapchain(const Swapchain&) = delete;
  Swapchain& operator=(const Swapchain&) = delete;

  Swapchain(Swapchain&&) = delete;
  Swapchain& operator=(Swapchain&&) = delete;

  void Resize(uint32_t width, uint32_t height);

  [[nodiscard]] vk::ResultValue<uint32_t> AcquireNextImage(
      vk::Semaphore signalSemaphore);

  [[nodiscard]] vk::Result Present(uint32_t imageIndex,
                                   vk::Semaphore waitSemaphore);

  [[nodiscard]] vk::Format GetImageFormat() const { return imageFormat_; }

  [[nodiscard]] vk::Extent2D GetExtent() const { return extent_; }

  [[nodiscard]] const std::vector<vk::UniqueImageView>& GetImageViews() const {
    return imageViews_;
  }

  [[nodiscard]] const std::vector<vk::Image>& GetImages() const {
    return images_;
  }

  [[nodiscard]] vk::SwapchainKHR GetSwapchainHandle() const {
    return swapchain_.get();
  }

  [[nodiscard]] uint32_t GetImageCount() const {
    return static_cast<uint32_t>(images_.size());
  }

  [[nodiscard]] float GetAspectRatio() const {
    return static_cast<float>(extent_.width) /
           static_cast<float>(extent_.height);
  }

  [[nodiscard]] const vk::Extent2D& GetSwapchainExtent() const {
    return extent_;
  }

 private:
  Context& context_;

  vk::UniqueSwapchainKHR swapchain_;

  vk::Format imageFormat_{vk::Format::eUndefined};
  vk::Extent2D extent_;

  std::vector<vk::Image> images_;
  std::vector<vk::UniqueImageView> imageViews_;
  void CreateSwapchain(uint32_t width, uint32_t height);

  void CreateImageViews();

  [[nodiscard]] static vk::SurfaceFormatKHR ChooseSurfaceFormat(
      const std::vector<vk::SurfaceFormatKHR>& availableFormats);

  [[nodiscard]] static vk::PresentModeKHR ChoosePresentMode(
      const std::vector<vk::PresentModeKHR>& availablePresentModes);

  [[nodiscard]] static vk::Extent2D ChooseSwapExtent(
      const vk::SurfaceCapabilitiesKHR& capabilities, uint32_t width,
      uint32_t height);
};
}  // namespace vulkan
