#pragma once

#include <memory>
#include <vector>

#include <vulkan/vulkan.hpp>

#include "rhi/pipeline.hpp"

namespace backends::vulkan {
class VulkanContext;

class VulkanPipelineLayout : public rhi::PipelineLayout {
 public:
  static std::unique_ptr<VulkanPipelineLayout> Create(
      VulkanContext& context,
      std::span<const rhi::DescriptorSetLayout* const> setLayouts,
      std::span<const rhi::PushConstantRange> pushConstantRanges = {});

  [[nodiscard]] const std::vector<rhi::DescriptorSetLayout*>& GetSetLayouts()
      const override {
    return setLayouts_;
  }

  [[nodiscard]] const std::vector<rhi::PushConstantRange>&
  GetPushConstantRanges() const override {
    return pushConstantRanges_;
  }

  [[nodiscard]] vk::PipelineLayout GetLayout() const { return layout_.get(); }

 private:
  VulkanPipelineLayout(std::vector<rhi::DescriptorSetLayout*> setLayouts,
                       std::vector<rhi::PushConstantRange> pushConstantRanges,
                       vk::UniquePipelineLayout layout);

  std::vector<rhi::DescriptorSetLayout*> setLayouts_;
  std::vector<rhi::PushConstantRange> pushConstantRanges_;
  vk::UniquePipelineLayout layout_;
};

class VulkanPipeline : public rhi::Pipeline {
 public:
  static std::unique_ptr<VulkanPipeline> Create(
      VulkanContext& context, const rhi::GraphicsPipelineDesc& desc);

  [[nodiscard]] const rhi::PipelineLayout& GetLayout() const override {
    return *layout_;
  }
  [[nodiscard]] bool IsGraphics() const override { return isGraphics_; }
  [[nodiscard]] vk::Pipeline GetPipeline() const { return pipeline_.get(); }

 private:
  VulkanPipeline(std::unique_ptr<VulkanPipelineLayout> layout,
                 vk::UniquePipeline pipeline, bool isGraphics);

  std::unique_ptr<VulkanPipelineLayout> layout_;
  vk::UniquePipeline pipeline_;
  bool isGraphics_;
};
}  // namespace backends::vulkan
