#include "backends/vulkan/vulkan_sampler.hpp"

#include <utility>

#include "backends/vulkan/vulkan_context.hpp"

namespace {
vk::Filter ToVkFilter(rhi::Filter filter) {
  return (filter == rhi::Filter::Nearest) ? vk::Filter::eNearest
                                          : vk::Filter::eLinear;
}

vk::SamplerAddressMode ToVkAddressMode(rhi::AddressMode addressMode) {
  return (addressMode == rhi::AddressMode::Repeat)
             ? vk::SamplerAddressMode::eRepeat
             : vk::SamplerAddressMode::eClampToEdge;
}
}  // namespace

namespace backends::vulkan {
std::unique_ptr<VulkanSampler> VulkanSampler::Create(
    VulkanContext& context, rhi::Filter magFilter, rhi::Filter minFilter,
    rhi::AddressMode addressModeU, rhi::AddressMode addressModeV) {
  // Map RHI to Vulkan
  vk::Filter vkMagFilter{ToVkFilter(magFilter)};
  vk::Filter vkMinFilter{ToVkFilter(minFilter)};

  vk::SamplerAddressMode vkAddressU{ToVkAddressMode(addressModeU)};
  vk::SamplerAddressMode vkAddressV{ToVkAddressMode(addressModeV)};
  vk::SamplerCreateInfo createInfo{
      .magFilter = vkMagFilter,
      .minFilter = vkMinFilter,
      .addressModeU = vkAddressU,
      .addressModeV = vkAddressV,
      .addressModeW = vk::SamplerAddressMode::eClampToEdge,  // Default for W
      .anisotropyEnable = VK_FALSE,  // Can add options later
      .maxAnisotropy = 1.0F,
      .compareEnable = VK_FALSE,
      .compareOp = vk::CompareOp::eAlways,
      .minLod = 0.0F,
      .maxLod = VK_LOD_CLAMP_NONE,
      .borderColor = vk::BorderColor::eIntOpaqueBlack,
      .unnormalizedCoordinates = VK_FALSE,
  };

  vk::UniqueSampler sampler{
      context.GetDevice().createSamplerUnique(createInfo)};

  return std::unique_ptr<VulkanSampler>(new VulkanSampler(
      magFilter, minFilter, addressModeU, addressModeV, std::move(sampler)));
}

VulkanSampler::VulkanSampler(rhi::Filter magFilter, rhi::Filter minFilter,
                             rhi::AddressMode addressModeU,
                             rhi::AddressMode addressModeV,
                             vk::UniqueSampler sampler)
    : magFilter_{magFilter},
      minFilter_{minFilter},
      addressModeU_{addressModeU},
      addressModeV_{addressModeV},
      sampler_{std::move(sampler)} {}
}  // namespace backends::vulkan
