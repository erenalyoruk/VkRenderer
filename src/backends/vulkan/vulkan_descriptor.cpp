#include "backends/vulkan/vulkan_descriptor.hpp"

#include <bit>
#include <utility>

#include "backends/vulkan/vulkan_buffer.hpp"
#include "backends/vulkan/vulkan_context.hpp"
#include "backends/vulkan/vulkan_sampler.hpp"
#include "backends/vulkan/vulkan_texture.hpp"

namespace backends::vulkan {
std::unique_ptr<VulkanDescriptorSetLayout> VulkanDescriptorSetLayout::Create(
    VulkanContext& context, std::span<const rhi::DescriptorBinding> bindings) {
  std::vector<vk::DescriptorSetLayoutBinding> vkBindings;
  for (const auto& binding : bindings) {
    vk::DescriptorType type{vk::DescriptorType::eUniformBuffer};
    switch (binding.type) {
      case rhi::DescriptorType::UniformBuffer:
        type = vk::DescriptorType::eUniformBuffer;
        break;
      case rhi::DescriptorType::StorageBuffer:
        type = vk::DescriptorType::eStorageBuffer;
        break;
      case rhi::DescriptorType::SampledImage:
        type = vk::DescriptorType::eSampledImage;
        break;
      case rhi::DescriptorType::Sampler:
        type = vk::DescriptorType::eSampler;
        break;
      case rhi::DescriptorType::CombinedImageSampler:
        type = vk::DescriptorType::eCombinedImageSampler;
        break;
      case rhi::DescriptorType::StorageImage:
        type = vk::DescriptorType::eStorageImage;
        break;
      default:
        type = vk::DescriptorType::eUniformBuffer;
        break;
    }

    vkBindings.push_back({
        .binding = binding.binding,
        .descriptorType = type,
        .descriptorCount = binding.count,
        .stageFlags =
            vk::ShaderStageFlagBits::eAll,  // All stages for simplicity
    });
  }

  vk::DescriptorSetLayoutCreateInfo createInfo{
      .bindingCount = static_cast<uint32_t>(vkBindings.size()),
      .pBindings = vkBindings.data(),
  };

  vk::UniqueDescriptorSetLayout layout =
      context.GetDevice().createDescriptorSetLayoutUnique(createInfo);

  std::vector<rhi::DescriptorBinding> bindingsCopy(bindings.begin(),
                                                   bindings.end());

  return std::unique_ptr<VulkanDescriptorSetLayout>(
      new VulkanDescriptorSetLayout(std::move(bindingsCopy),
                                    std::move(layout)));
}

VulkanDescriptorSetLayout::VulkanDescriptorSetLayout(
    std::vector<rhi::DescriptorBinding> bindings,
    vk::UniqueDescriptorSetLayout layout)
    : bindings_{std::move(bindings)}, layout_{std::move(layout)} {}

std::unique_ptr<VulkanDescriptorSet> VulkanDescriptorSet::Create(
    VulkanContext& context, const VulkanDescriptorSetLayout* layout) {
  // Assume descriptor pool exists in context
  vk::DescriptorSetAllocateInfo allocInfo{
      .descriptorPool = context.GetDescriptorPool(),  // Add to VulkanContext
      .descriptorSetCount = 1,
      .pSetLayouts = &layout->GetLayout(),
  };

  vk::DescriptorSet set =
      context.GetDevice().allocateDescriptorSets(allocInfo)[0];

  return std::unique_ptr<VulkanDescriptorSet>(
      new VulkanDescriptorSet(set, context.GetDevice()));
}

VulkanDescriptorSet::VulkanDescriptorSet(vk::DescriptorSet set,
                                         vk::Device device)
    : set_{set}, device_{device} {}

void VulkanDescriptorSet::BindBuffer(uint32_t binding,
                                     const rhi::Buffer* buffer,
                                     rhi::Size offset, rhi::Size range) {
  const auto* vkBuffer{std::bit_cast<const VulkanBuffer*>(buffer)};
  vk::DescriptorBufferInfo bufferInfo{
      .buffer = vkBuffer->GetHandle(),
      .offset = offset,
      .range = range == 0 ? VK_WHOLE_SIZE : range,
  };

  vk::WriteDescriptorSet write{
      .dstSet = set_,
      .dstBinding = binding,
      .descriptorCount = 1,
      .descriptorType =
          vk::DescriptorType::eUniformBuffer,  // Or detect from layout
      .pBufferInfo = &bufferInfo,
  };

  device_.updateDescriptorSets(write, {});
}

void VulkanDescriptorSet::BindTexture(uint32_t binding,
                                      const rhi::Texture* texture,
                                      const rhi::Sampler* sampler) {
  const auto* vkTexture{std::bit_cast<const VulkanTexture*>(texture)};
  const auto* vkSampler{(sampler != nullptr)
                            ? std::bit_cast<const VulkanSampler*>(sampler)
                            : nullptr};

  vk::DescriptorImageInfo imageInfo{
      .sampler =
          (vkSampler != nullptr) ? vkSampler->GetSampler() : VK_NULL_HANDLE,
      .imageView = vkTexture->GetImageView(),
      .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
  };

  // Use CombinedImageSampler when sampler is provided, otherwise SampledImage
  vk::DescriptorType descriptorType =
      (vkSampler != nullptr) ? vk::DescriptorType::eCombinedImageSampler
                             : vk::DescriptorType::eSampledImage;

  vk::WriteDescriptorSet write{
      .dstSet = set_,
      .dstBinding = binding,
      .descriptorCount = 1,
      .descriptorType = descriptorType,
      .pImageInfo = &imageInfo,
  };

  device_.updateDescriptorSets(write, {});
}
}  // namespace backends::vulkan
