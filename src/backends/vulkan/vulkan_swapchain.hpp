#pragma once

#include <memory>
#include <vector>

#include <vulkan/vulkan.hpp>

#include "rhi/swapchain.hpp"

namespace backends::vulkan {
class VulkanContext;

class VulkanSwapchain : public rhi::Swapchain {
 public:
  static std::unique_ptr<VulkanSwapchain> Create(VulkanContext& context,
                                                 uint32_t width,
                                                 uint32_t height,
                                                 rhi::Format format);

  void Present(uint32_t imageIndex, rhi::Semaphore* waitSemaphore) override;
  void Resize(uint32_t width, uint32_t height) override;
  [[nodiscard]] uint32_t AcquireNextImage(
      rhi::Semaphore* signalSemaphore) override;
  [[nodiscard]] const std::vector<rhi::Texture*>& GetImages() const override;
  [[nodiscard]] uint32_t GetImageCount() const override {
    return static_cast<uint32_t>(images_.size());
  }
  [[nodiscard]] uint32_t GetWidth() const override { return extent_.width; }
  [[nodiscard]] uint32_t GetHeight() const override { return extent_.height; }

  [[nodiscard]] vk::SwapchainKHR GetSwapchain() const {
    return swapchain_.get();
  }

 private:
  VulkanSwapchain(VulkanContext& context, vk::UniqueSwapchainKHR swapchain,
                  vk::Extent2D extent,
                  std::vector<std::unique_ptr<rhi::Texture>> images);

  VulkanContext& context_;  // NOLINT
  vk::UniqueSwapchainKHR swapchain_;
  vk::Extent2D extent_;
  std::vector<std::unique_ptr<rhi::Texture>> images_;
  std::vector<rhi::Texture*> imagePtrs_;  // For GetImages
};
}  // namespace backends::vulkan
