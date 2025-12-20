#include "backends/vulkan/vulkan_texture.hpp"

#include <bit>
#include <utility>

#include "backends/vulkan/vulkan_buffer.hpp"
#include "backends/vulkan/vulkan_context.hpp"

namespace {
vk::Format ToVkFormat(rhi::Format format) {
  switch (format) {
    case rhi::Format::R8Unorm:
      return vk::Format::eR8Unorm;
    case rhi::Format::R8G8Unorm:
      return vk::Format::eR8G8Unorm;
    case rhi::Format::R8G8B8Unorm:
      return vk::Format::eR8G8B8Unorm;
    case rhi::Format::R8G8B8A8Unorm:
      return vk::Format::eR8G8B8A8Unorm;
    case rhi::Format::R8G8B8A8Srgb:
      return vk::Format::eR8G8B8A8Srgb;
    case rhi::Format::B8G8R8A8Unorm:
      return vk::Format::eB8G8R8A8Unorm;
    case rhi::Format::B8G8R8A8Srgb:
      return vk::Format::eB8G8R8A8Srgb;
    case rhi::Format::R16Sfloat:
      return vk::Format::eR16Sfloat;
    case rhi::Format::R16G16Sfloat:
      return vk::Format::eR16G16Sfloat;
    case rhi::Format::R16G16B16A16Sfloat:
      return vk::Format::eR16G16B16A16Sfloat;
    case rhi::Format::R32Sfloat:
      return vk::Format::eR32Sfloat;
    case rhi::Format::R32G32Sfloat:
      return vk::Format::eR32G32Sfloat;
    case rhi::Format::R32G32B32Sfloat:
      return vk::Format::eR32G32B32Sfloat;
    case rhi::Format::R32G32B32A32Sfloat:
      return vk::Format::eR32G32B32A32Sfloat;
    case rhi::Format::D16Unorm:
      return vk::Format::eD16Unorm;
    case rhi::Format::D32Sfloat:
      return vk::Format::eD32Sfloat;
    case rhi::Format::D24UnormS8Uint:
      return vk::Format::eD24UnormS8Uint;
    case rhi::Format::D32SfloatS8Uint:
      return vk::Format::eD32SfloatS8Uint;
    default:
      return vk::Format::eUndefined;
  }
}

bool IsDepthFormat(rhi::Format format) {
  return format == rhi::Format::D16Unorm || format == rhi::Format::D32Sfloat ||
         format == rhi::Format::D24UnormS8Uint ||
         format == rhi::Format::D32SfloatS8Uint;
}
}  // namespace

namespace backends::vulkan {
VulkanTexture::VulkanTexture(VulkanContext& context, uint32_t width,
                             uint32_t height, rhi::Format format,
                             vk::Image image, vk::UniqueImageView imageView)
    : context_{context},
      width_{width},
      height_{height},
      format_{format},
      image_{image},
      allocation_{VK_NULL_HANDLE},
      imageView_{std::move(imageView)} {}

VulkanTexture::~VulkanTexture() {
  // Only destroy if we own the allocation (not swapchain images)
  if (allocation_ != VK_NULL_HANDLE) {
    vmaDestroyImage(context_.GetAllocator().GetHandle(), image_, allocation_);
  }
}

std::unique_ptr<VulkanTexture> VulkanTexture::Create(VulkanContext& context,
                                                     uint32_t width,
                                                     uint32_t height,
                                                     rhi::Format format,
                                                     rhi::TextureUsage usage) {
  vk::Format vkFormat{ToVkFormat(format)};

  vk::ImageUsageFlags vkUsage;
  if ((usage & rhi::TextureUsage::Sampled) != rhi::TextureUsage{0}) {
    vkUsage |= vk::ImageUsageFlagBits::eSampled;
  }
  if ((usage & rhi::TextureUsage::Storage) != rhi::TextureUsage{0}) {
    vkUsage |= vk::ImageUsageFlagBits::eStorage;
  }
  if ((usage & rhi::TextureUsage::ColorAttachment) != rhi::TextureUsage{0}) {
    vkUsage |= vk::ImageUsageFlagBits::eColorAttachment;
  }
  if ((usage & rhi::TextureUsage::DepthStencilAttachment) !=
      rhi::TextureUsage{0}) {
    vkUsage |= vk::ImageUsageFlagBits::eDepthStencilAttachment;
  }

  vkUsage |= vk::ImageUsageFlagBits::eTransferDst;  // For uploads

  vk::ImageCreateInfo imageInfo{
      .imageType = vk::ImageType::e2D,
      .format = vkFormat,
      .extent = {.width = width, .height = height, .depth = 1},
      .mipLevels = 1,
      .arrayLayers = 1,
      .samples = vk::SampleCountFlagBits::e1,
      .tiling = vk::ImageTiling::eOptimal,
      .usage = vkUsage,
      .initialLayout = vk::ImageLayout::eUndefined,
  };

  VmaAllocationCreateInfo allocInfo{
      .usage = VMA_MEMORY_USAGE_GPU_ONLY,
  };

  VkImage rawImage{VK_NULL_HANDLE};
  VmaAllocation allocation{VK_NULL_HANDLE};
  VkResult res = vmaCreateImage(context.GetAllocator().GetHandle(),
                                std::bit_cast<VkImageCreateInfo*>(&imageInfo),
                                &allocInfo, &rawImage, &allocation, nullptr);
  if (res != VK_SUCCESS) {
    return nullptr;
  }

  vk::Image image{rawImage};

  vk::ImageViewCreateInfo viewInfo{
      .image = image,
      .viewType = vk::ImageViewType::e2D,
      .format = vkFormat,
      .subresourceRange =
          {
              .aspectMask = IsDepthFormat(format)
                                ? vk::ImageAspectFlagBits::eDepth
                                : vk::ImageAspectFlagBits::eColor,
              .levelCount = 1,
              .layerCount = 1,
          },
  };

  vk::UniqueImageView imageView =
      context.GetDevice().createImageViewUnique(viewInfo);

  return std::unique_ptr<VulkanTexture>(new VulkanTexture(
      context, width, height, format, image, allocation, std::move(imageView)));
}

VulkanTexture::VulkanTexture(VulkanContext& context, uint32_t width,
                             uint32_t height, rhi::Format format,
                             vk::Image image, VmaAllocation allocation,
                             vk::UniqueImageView imageView)
    : context_{context},
      width_{width},
      height_{height},
      format_{format},
      image_{image},
      allocation_{allocation},
      imageView_{std::move(imageView)} {}

void VulkanTexture::Upload(std::span<const std::byte> data, uint32_t mipLevel,
                           uint32_t arrayLayer) {
  if (allocation_ == VK_NULL_HANDLE) {  // Swapchain images can't be uploaded to
    return;
  }

  if (data.empty()) {
    return;
  }

  // Create staging buffer and upload data to it
  auto staging = VulkanBuffer::Create(context_.GetAllocator(), data.size(),
                                      rhi::BufferUsage::TransferSrc,
                                      rhi::MemoryUsage::CPUToGPU);
  staging->Upload(data, 0);

  // Create a temporary command pool and command buffer for the upload
  vk::CommandPoolCreateInfo poolInfo{
      .flags = vk::CommandPoolCreateFlagBits::eTransient,
      .queueFamilyIndex = context_.GetGraphicsFamilyIndex(),
  };

  vk::UniqueCommandPool commandPool =
      context_.GetDevice().createCommandPoolUnique(poolInfo);

  vk::CommandBufferAllocateInfo allocInfo{
      .commandPool = commandPool.get(),
      .level = vk::CommandBufferLevel::ePrimary,
      .commandBufferCount = 1,
  };

  std::vector<vk::CommandBuffer> commandBuffers =
      context_.GetDevice().allocateCommandBuffers(allocInfo);
  vk::CommandBuffer cmd = commandBuffers[0];

  // Begin recording
  vk::CommandBufferBeginInfo beginInfo{
      .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
  };
  cmd.begin(beginInfo);

  // Determine aspect mask
  vk::ImageAspectFlags aspectMask = IsDepthFormat(format_)
                                        ? vk::ImageAspectFlagBits::eDepth
                                        : vk::ImageAspectFlagBits::eColor;

  // Transition image from Undefined to TransferDst
  vk::ImageMemoryBarrier toTransferBarrier{
      .srcAccessMask = vk::AccessFlags{},
      .dstAccessMask = vk::AccessFlagBits::eTransferWrite,
      .oldLayout = vk::ImageLayout::eUndefined,
      .newLayout = vk::ImageLayout::eTransferDstOptimal,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = image_,
      .subresourceRange =
          {
              .aspectMask = aspectMask,
              .baseMipLevel = mipLevel,
              .levelCount = 1,
              .baseArrayLayer = arrayLayer,
              .layerCount = 1,
          },
  };

  cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe,
                      vk::PipelineStageFlagBits::eTransfer, {}, {}, {},
                      toTransferBarrier);

  // Copy buffer to image
  vk::BufferImageCopy copyRegion{
      .bufferOffset = 0,
      .bufferRowLength = 0,
      .bufferImageHeight = 0,
      .imageSubresource =
          {
              .aspectMask = aspectMask,
              .mipLevel = mipLevel,
              .baseArrayLayer = arrayLayer,
              .layerCount = 1,
          },
      .imageOffset = {.x = 0, .y = 0, .z = 0},
      .imageExtent = {.width = width_, .height = height_, .depth = 1},
  };

  cmd.copyBufferToImage(staging->GetHandle(), image_,
                        vk::ImageLayout::eTransferDstOptimal, copyRegion);

  // Transition image from TransferDst to ShaderReadOnly
  vk::ImageMemoryBarrier toShaderReadBarrier{
      .srcAccessMask = vk::AccessFlagBits::eTransferWrite,
      .dstAccessMask = vk::AccessFlagBits::eShaderRead,
      .oldLayout = vk::ImageLayout::eTransferDstOptimal,
      .newLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = image_,
      .subresourceRange =
          {
              .aspectMask = aspectMask,
              .baseMipLevel = mipLevel,
              .levelCount = 1,
              .baseArrayLayer = arrayLayer,
              .layerCount = 1,
          },
  };

  cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                      vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {},
                      toShaderReadBarrier);

  // End recording
  cmd.end();

  // Submit and wait
  vk::SubmitInfo submitInfo{
      .commandBufferCount = 1,
      .pCommandBuffers = &cmd,
  };

  vk::Queue graphicsQueue =
      context_.GetDevice().getQueue(context_.GetGraphicsFamilyIndex(), 0);
  graphicsQueue.submit(submitInfo);
  graphicsQueue.waitIdle();

  // Command pool and staging buffer are automatically cleaned up
}
}  // namespace backends::vulkan
