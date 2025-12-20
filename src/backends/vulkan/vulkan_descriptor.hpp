#pragma once

#include <memory>
#include <vector>

#include <vulkan/vulkan.hpp>

#include "rhi/descriptor.hpp"

namespace backends::vulkan {
class VulkanContext;

class VulkanDescriptorSetLayout : public rhi::DescriptorSetLayout {
 public:
  static std::unique_ptr<VulkanDescriptorSetLayout> Create(
      VulkanContext& context, std::span<const rhi::DescriptorBinding> bindings);

  [[nodiscard]] const std::vector<rhi::DescriptorBinding>& GetBindings()
      const override {
    return bindings_;
  }

  [[nodiscard]] const vk::DescriptorSetLayout& GetLayout() const {
    return layout_.get();
  }

 private:
  VulkanDescriptorSetLayout(std::vector<rhi::DescriptorBinding> bindings,
                            vk::UniqueDescriptorSetLayout layout);

  std::vector<rhi::DescriptorBinding> bindings_;
  vk::UniqueDescriptorSetLayout layout_;
};

class VulkanDescriptorSet : public rhi::DescriptorSet {
 public:
  static std::unique_ptr<VulkanDescriptorSet> Create(
      VulkanContext& context, const VulkanDescriptorSetLayout* layout);

  void BindBuffer(uint32_t binding, const rhi::Buffer* buffer,
                  rhi::Size offset = 0, rhi::Size range = 0) override;

  void BindStorageBuffer(uint32_t binding, const rhi::Buffer* buffer,
                         rhi::Size offset = 0, rhi::Size range = 0) override;

  void BindTexture(uint32_t binding, const rhi::Texture* texture,
                   const rhi::Sampler* sampler, uint32_t arrayElement) override;

  [[nodiscard]] vk::DescriptorSet GetSet() const { return set_; }

 private:
  VulkanDescriptorSet(vk::DescriptorSet set, vk::Device device);

  vk::DescriptorSet set_;
  vk::Device device_;
  std::vector<vk::WriteDescriptorSet> writes_;
};
}  // namespace backends::vulkan
