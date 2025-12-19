#include "vulkan/swapchain.hpp"

#include <algorithm>
#include <array>

#include "logger.hpp"

namespace vulkan {
Swapchain::Swapchain(Context& context, uint32_t width, uint32_t height)
    : context_{context} {
  CreateSwapchain(width, height);
  CreateImageViews();

  LOG_DEBUG("Swapchain created with {} images of size {}x{}", images_.size(),
            extent_.width, extent_.height);
}

void Swapchain::Resize(uint32_t width, uint32_t height) {
  if (width == 0 || height == 0) {
    return;
  }

  context_.GetDevice().waitIdle();

  imageViews_.clear();
  swapchain_.reset();

  CreateSwapchain(width, height);
  CreateImageViews();
}

void Swapchain::CreateSwapchain(uint32_t width, uint32_t height) {
  auto physicalDevice{context_.GetPhysicalDevice()};

  auto capabilities{
      physicalDevice.getSurfaceCapabilitiesKHR(context_.GetSurface())};
  auto formats{physicalDevice.getSurfaceFormatsKHR(context_.GetSurface())};
  auto presentModes{
      physicalDevice.getSurfacePresentModesKHR(context_.GetSurface())};

  vk::SurfaceFormatKHR surfaceFormat{ChooseSurfaceFormat(formats)};
  vk::PresentModeKHR presentMode{ChoosePresentMode(presentModes)};
  vk::Extent2D extent{ChooseSwapExtent(capabilities, width, height)};

  uint32_t imageCount{capabilities.minImageCount + 1};
  if (capabilities.maxImageCount > 0 &&
      imageCount > capabilities.maxImageCount) {
    imageCount = capabilities.maxImageCount;
  }

  vk::SwapchainCreateInfoKHR createInfo{
      .surface = context_.GetSurface(),
      .minImageCount = imageCount,
      .imageFormat = surfaceFormat.format,
      .imageColorSpace = surfaceFormat.colorSpace,
      .imageExtent = extent,
      .imageArrayLayers = 1,
      .imageUsage = vk::ImageUsageFlagBits::eColorAttachment |
                    vk::ImageUsageFlagBits::eTransferDst,
      .preTransform = capabilities.currentTransform,
      .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
      .presentMode = presentMode,
      .clipped = VK_TRUE,
      .oldSwapchain = nullptr,
  };

  auto indices{context_.GetQueueFamilyIndices()};
  std::array<uint32_t, 3> queueFamilyIndices{indices.graphicsFamily.value(),
                                             indices.computeFamily.value(),
                                             indices.transferFamily.value()};

  if (indices.graphicsFamily != indices.computeFamily ||
      indices.graphicsFamily != indices.transferFamily ||
      indices.computeFamily != indices.transferFamily) {
    createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
    createInfo.queueFamilyIndexCount =
        static_cast<uint32_t>(queueFamilyIndices.size());
    createInfo.pQueueFamilyIndices = queueFamilyIndices.data();
  } else {
    createInfo.imageSharingMode = vk::SharingMode::eExclusive;
    createInfo.queueFamilyIndexCount = 0;
    createInfo.pQueueFamilyIndices = nullptr;
  }

  swapchain_ = context_.GetDevice().createSwapchainKHRUnique(createInfo);

  imageFormat_ = surfaceFormat.format;
  extent_ = extent;

  images_ = context_.GetDevice().getSwapchainImagesKHR(swapchain_.get());
}

void Swapchain::CreateImageViews() {
  imageViews_.clear();
  imageViews_.reserve(images_.size());

  for (const auto& image : images_) {
    vk::ImageViewCreateInfo createInfo{
        .image = image,
        .viewType = vk::ImageViewType::e2D,
        .format = imageFormat_,
        .components =
            vk::ComponentMapping{
                .r = vk::ComponentSwizzle::eIdentity,
                .g = vk::ComponentSwizzle::eIdentity,
                .b = vk::ComponentSwizzle::eIdentity,
                .a = vk::ComponentSwizzle::eIdentity,
            },
        .subresourceRange =
            vk::ImageSubresourceRange{
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
    };

    imageViews_.emplace_back(
        context_.GetDevice().createImageViewUnique(createInfo));
  }
}

vk::ResultValue<uint32_t> Swapchain::AcquireNextImage(vk::Semaphore semaphore) {
  return context_.GetDevice().acquireNextImageKHR(swapchain_.get(), UINT64_MAX,
                                                  semaphore, nullptr);
}

vk::Result Swapchain::Present(uint32_t imageIndex,
                              vk::Semaphore waitSemaphore) {
  vk::PresentInfoKHR presentInfo{
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = &waitSemaphore,
      .swapchainCount = 1,
      .pSwapchains = &swapchain_.get(),
      .pImageIndices = &imageIndex,
      .pResults = nullptr,
  };

  auto graphicsQueue{context_.GetGraphicsQueue()};
  return graphicsQueue.presentKHR(presentInfo);
}

vk::SurfaceFormatKHR Swapchain::ChooseSurfaceFormat(
    const std::vector<vk::SurfaceFormatKHR>& availableFormats) {
  for (const auto& availableFormat : availableFormats) {
    if (availableFormat.format == vk::Format::eB8G8R8A8Srgb &&
        availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
      return availableFormat;
    }
  }

  return availableFormats[0];
}

vk::PresentModeKHR Swapchain::ChoosePresentMode(
    [[maybe_unused]] const std::vector<vk::PresentModeKHR>&
        availablePresentModes) {
  // for (const auto& available_present_mode : available_present_modes) {
  //   if (available_present_mode == vk::PresentModeKHR::eMailbox) {
  //     return available_present_mode;
  //   }
  // }

  return vk::PresentModeKHR::eFifo;
}

vk::Extent2D Swapchain::ChooseSwapExtent(
    const vk::SurfaceCapabilitiesKHR& capabilities, uint32_t width,
    uint32_t height) {
  if (capabilities.currentExtent.width != UINT32_MAX) {
    return capabilities.currentExtent;
  }

  vk::Extent2D actualExtent{
      .width = std::clamp(width, capabilities.minImageExtent.width,
                          capabilities.maxImageExtent.width),
      .height = std::clamp(height, capabilities.minImageExtent.height,
                           capabilities.maxImageExtent.height),
  };

  return actualExtent;
}
}  // namespace vulkan
