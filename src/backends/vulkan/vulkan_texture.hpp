#pragma once

#include <cstdint>
#include <memory>

#include <vulkan/vulkan.hpp>

#include "backends/vulkan/vulkan_allocator.hpp"
#include "rhi/texture.hpp"

namespace backends::vulkan {
class VulkanContext;

class VulkanTexture : public rhi::Texture {
 public:
  // Constructor for swapchain images without allocation
  VulkanTexture(VulkanContext& context, uint32_t width, uint32_t height,
                rhi::Format format, vk::Image image,
                vk::UniqueImageView imageView);

  ~VulkanTexture() override;

  static std::unique_ptr<VulkanTexture> Create(VulkanContext& context,
                                               uint32_t width, uint32_t height,
                                               rhi::Format format,
                                               rhi::TextureUsage usage);

  // RHI implementations
  void Upload(std::span<const std::byte> data, uint32_t mipLevel = 0,
              uint32_t arrayLayer = 0) override;
  [[nodiscard]] rhi::Format GetFormat() const override { return format_; }
  [[nodiscard]] uint32_t GetWidth() const override { return width_; }
  [[nodiscard]] uint32_t GetHeight() const override { return height_; }
  [[nodiscard]] uint32_t GetDepth() const override { return 1; }
  [[nodiscard]] uint32_t GetMipLevels() const override { return 1; }

  // Vulkan getters
  [[nodiscard]] vk::Image GetImage() const { return image_; }
  [[nodiscard]] vk::ImageView GetImageView() const { return imageView_.get(); }

 private:
  VulkanTexture(VulkanContext& context, uint32_t width, uint32_t height,
                rhi::Format format, vk::Image image, VmaAllocation allocation,
                vk::UniqueImageView imageView);

  VulkanContext& context_;  // NOLINT

  uint32_t width_;
  uint32_t height_;
  rhi::Format format_;
  vk::Image image_;
  VmaAllocation allocation_;
  vk::UniqueImageView imageView_;
};
}  // namespace backends::vulkan
