#include "backends/vulkan/vulkan_swapchain.hpp"

#include <algorithm>
#include <bit>
#include <memory>

#include "backends/vulkan/vulkan_context.hpp"
#include "backends/vulkan/vulkan_semaphore.hpp"
#include "backends/vulkan/vulkan_texture.hpp"

namespace {
rhi::Format VkFormatToRhiFormat(vk::Format format) {
  switch (format) {
    case vk::Format::eR8G8B8A8Unorm:
      return rhi::Format::R8G8B8A8Unorm;
    case vk::Format::eR8G8B8A8Srgb:
      return rhi::Format::R8G8B8A8Srgb;
    case vk::Format::eB8G8R8A8Unorm:
      return rhi::Format::B8G8R8A8Unorm;
    case vk::Format::eB8G8R8A8Srgb:
      return rhi::Format::B8G8R8A8Srgb;
    default:
      return rhi::Format::R8G8B8A8Unorm;
  }
}
}  // namespace

namespace backends::vulkan {
std::unique_ptr<VulkanSwapchain> VulkanSwapchain::Create(
    VulkanContext& context, uint32_t width, uint32_t height,
    rhi::Format /*format*/) {
  vk::SurfaceCapabilitiesKHR capabilities =
      context.GetPhysicalDevice().getSurfaceCapabilitiesKHR(
          context.GetSurface());
  auto surfaceFormats =
      context.GetPhysicalDevice().getSurfaceFormatsKHR(context.GetSurface());
  auto presentModes = context.GetPhysicalDevice().getSurfacePresentModesKHR(
      context.GetSurface());

  vk::SurfaceFormatKHR surfaceFormat = surfaceFormats[0];
  for (const auto& fmt : surfaceFormats) {
    if (fmt.format == vk::Format::eB8G8R8A8Unorm &&
        fmt.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
      surfaceFormat = fmt;
      break;
    }
  }

  vk::PresentModeKHR presentMode = vk::PresentModeKHR::eFifo;
  // for (const auto& mode : presentModes) {
  //   if (mode == vk::PresentModeKHR::eMailbox) {
  //     presentMode = mode;
  //     break;
  //   }
  // }

  vk::Extent2D extent{
      .width = std::clamp(width, capabilities.minImageExtent.width,
                          capabilities.maxImageExtent.width),
      .height = std::clamp(height, capabilities.minImageExtent.height,
                           capabilities.maxImageExtent.height),
  };

  uint32_t imageCount = capabilities.minImageCount + 1;
  if (capabilities.maxImageCount > 0 &&
      imageCount > capabilities.maxImageCount) {
    imageCount = capabilities.maxImageCount;
  }

  vk::SwapchainCreateInfoKHR createInfo{
      .surface = context.GetSurface(),
      .minImageCount = imageCount,
      .imageFormat = surfaceFormat.format,
      .imageColorSpace = surfaceFormat.colorSpace,
      .imageExtent = extent,
      .imageArrayLayers = 1,
      .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
      .imageSharingMode = vk::SharingMode::eExclusive,
      .preTransform = capabilities.currentTransform,
      .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
      .presentMode = presentMode,
      .clipped = VK_TRUE,
  };

  vk::UniqueSwapchainKHR swapchain =
      context.GetDevice().createSwapchainKHRUnique(createInfo);

  auto swapchainImages =
      context.GetDevice().getSwapchainImagesKHR(swapchain.get());

  // Convert Vulkan format to RHI format
  rhi::Format rhiFormat = VkFormatToRhiFormat(surfaceFormat.format);

  std::vector<std::unique_ptr<rhi::Texture>> images;
  for (auto image : swapchainImages) {
    vk::ImageViewCreateInfo viewInfo{
        .image = image,
        .viewType = vk::ImageViewType::e2D,
        .format = surfaceFormat.format,
        .subresourceRange =
            {
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .levelCount = 1,
                .layerCount = 1,
            },
    };
    vk::UniqueImageView imageView =
        context.GetDevice().createImageViewUnique(viewInfo);

    auto texture =
        std::make_unique<VulkanTexture>(context, extent.width, extent.height,
                                        rhiFormat, image, std::move(imageView));
    images.push_back(std::move(texture));
  }

  return std::unique_ptr<VulkanSwapchain>(new VulkanSwapchain(
      context, std::move(swapchain), extent, std::move(images)));
}

VulkanSwapchain::VulkanSwapchain(
    VulkanContext& context, vk::UniqueSwapchainKHR swapchain,
    vk::Extent2D extent, std::vector<std::unique_ptr<rhi::Texture>> images)
    : context_{context},
      swapchain_{std::move(swapchain)},
      extent_{extent},
      images_{std::move(images)} {
  for (auto& img : images_) {
    imagePtrs_.push_back(img.get());
  }
}

uint32_t VulkanSwapchain::AcquireNextImage(rhi::Semaphore* signalSemaphore) {
  try {
    vk::AcquireNextImageInfoKHR acquireInfo{
        .swapchain = swapchain_.get(),
        .timeout = UINT64_MAX,
        .semaphore = (signalSemaphore != nullptr)
                         ? std::bit_cast<VulkanSemaphore*>(signalSemaphore)
                               ->GetSemaphore()
                         : VK_NULL_HANDLE,
        .deviceMask = 1,
    };

    uint32_t imageIndex{0};
    vk::Result result =
        context_.GetDevice().acquireNextImage2KHR(&acquireInfo, &imageIndex);

    // Return UINT32_MAX to signal that the swapchain needs recreation
    if (result == vk::Result::eErrorOutOfDateKHR ||
        result == vk::Result::eSuboptimalKHR) {
      return UINT32_MAX;
    }

    return imageIndex;
  } catch (const vk::OutOfDateKHRError&) {
    return UINT32_MAX;
  }
}

void VulkanSwapchain::Present(uint32_t /*imageIndex*/,
                              rhi::Semaphore* /*waitSemaphore*/) {
  // Present is handled in Queue::Present
}

void VulkanSwapchain::Resize(uint32_t width, uint32_t height) {
  context_.GetDevice().waitIdle();
  swapchain_.reset();
  images_.clear();
  imagePtrs_.clear();

  auto newSwapchain = VulkanSwapchain::Create(context_, width, height,
                                              rhi::Format::B8G8R8A8Unorm);

  swapchain_ = std::move(newSwapchain->swapchain_);
  extent_ = newSwapchain->extent_;
  images_ = std::move(newSwapchain->images_);
  for (auto& img : images_) {
    imagePtrs_.push_back(img.get());
  }
}

const std::vector<rhi::Texture*>& VulkanSwapchain::GetImages() const {
  return imagePtrs_;
}
}  // namespace backends::vulkan
