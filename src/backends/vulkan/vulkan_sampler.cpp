#include "backends/vulkan/vulkan_sampler.hpp"

#include <utility>

#include "backends/vulkan/vulkan_context.hpp"

namespace {
vk::Filter ToVkFilter(rhi::Filter filter) {
  return (filter == rhi::Filter::Nearest) ? vk::Filter::eNearest
                                          : vk::Filter::eLinear;
}

vk::SamplerAddressMode ToVkAddressMode(rhi::AddressMode addressMode) {
  switch (addressMode) {
    case rhi::AddressMode::Repeat:
      return vk::SamplerAddressMode::eRepeat;
    case rhi::AddressMode::ClampToEdge:
      return vk::SamplerAddressMode::eClampToEdge;
    case rhi::AddressMode::ClampToBorder:
      return vk::SamplerAddressMode::eClampToBorder;
    default:
      return vk::SamplerAddressMode::eClampToEdge;
  }
}

vk::CompareOp ToVkCompareOp(rhi::CompareOp op) {
  switch (op) {
    case rhi::CompareOp::Less:
      return vk::CompareOp::eLess;
    case rhi::CompareOp::LessOrEqual:
      return vk::CompareOp::eLessOrEqual;
    case rhi::CompareOp::Greater:
      return vk::CompareOp::eGreater;
    case rhi::CompareOp::GreaterOrEqual:
      return vk::CompareOp::eGreaterOrEqual;
    case rhi::CompareOp::Equal:
      return vk::CompareOp::eEqual;
    case rhi::CompareOp::NotEqual:
      return vk::CompareOp::eNotEqual;
    case rhi::CompareOp::Always:
      return vk::CompareOp::eAlways;
    case rhi::CompareOp::Never:
      return vk::CompareOp::eNever;
    default:
      return vk::CompareOp::eLess;
  }
}
}  // namespace

namespace backends::vulkan {
std::unique_ptr<VulkanSampler> VulkanSampler::Create(
    VulkanContext& context, rhi::Filter magFilter, rhi::Filter minFilter,
    rhi::AddressMode addressModeU, rhi::AddressMode addressModeV,
    std::optional<std::span<const float, 4>> borderColor, bool compareEnable,
    rhi::CompareOp compareOp) {
  // Map RHI to Vulkan
  vk::Filter vkMagFilter{ToVkFilter(magFilter)};
  vk::Filter vkMinFilter{ToVkFilter(minFilter)};

  vk::SamplerAddressMode vkAddressU{ToVkAddressMode(addressModeU)};
  vk::SamplerAddressMode vkAddressV{ToVkAddressMode(addressModeV)};

  vk::Bool32 vkCompareEnable = compareEnable ? VK_TRUE : VK_FALSE;

  vk::BorderColor borderColorValue{vk::BorderColor::eFloatTransparentBlack};
  if (borderColor.has_value()) {
    borderColorValue = vk::BorderColor::eFloatCustomEXT;
  }

  vk::SamplerCreateInfo createInfo{
      .magFilter = vkMagFilter,
      .minFilter = vkMinFilter,
      .addressModeU = vkAddressU,
      .addressModeV = vkAddressV,
      .addressModeW = vk::SamplerAddressMode::eClampToEdge,  // Default for W
      .anisotropyEnable = VK_FALSE,  // Can add options later
      .maxAnisotropy = 1.0F,
      .compareEnable = vkCompareEnable,
      .compareOp = ToVkCompareOp(compareOp),
      .minLod = 0.0F,
      .maxLod = VK_LOD_CLAMP_NONE,
      .borderColor = borderColorValue,
      .unnormalizedCoordinates = VK_FALSE,
  };

  if (borderColor.has_value()) {
    vk::SamplerCustomBorderColorCreateInfoEXT customBorderColorInfo{
        .customBorderColor =
            vk::ClearColorValue{
                .float32 = {{
                    borderColor.value()[0],
                    borderColor.value()[1],
                    borderColor.value()[2],
                    borderColor.value()[3],
                }},
            },
        .format = vk::Format::eR8G8B8A8Unorm,
    };
    createInfo.pNext = &customBorderColorInfo;
  }

  vk::UniqueSampler sampler{
      context.GetDevice().createSamplerUnique(createInfo)};

  return std::unique_ptr<VulkanSampler>(new VulkanSampler(
      magFilter, minFilter, addressModeU, addressModeV, borderColor,
      compareEnable, compareOp, std::move(sampler)));
}

VulkanSampler::VulkanSampler(
    rhi::Filter magFilter, rhi::Filter minFilter, rhi::AddressMode addressModeU,
    rhi::AddressMode addressModeV,
    std::optional<std::span<const float, 4>> borderColor, bool compareEnable,
    rhi::CompareOp compareOp, vk::UniqueSampler sampler)
    : magFilter_{magFilter},
      minFilter_{minFilter},
      addressModeU_{addressModeU},
      addressModeV_{addressModeV},
      compareEnable_{compareEnable},
      compareOp_{compareOp},
      sampler_{std::move(sampler)} {
  if (borderColor.has_value()) {
    borderColor_ = {borderColor.value()[0], borderColor.value()[1],
                    borderColor.value()[2], borderColor.value()[3]};
  }
}
}  // namespace backends::vulkan
