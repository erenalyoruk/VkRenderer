#pragma once

#include <memory>

#include <vulkan/vulkan.hpp>

#include "vulkan/context.hpp"
#include "vulkan/descriptors.hpp"
#include "vulkan/global_shader_data.hpp"
#include "vulkan/material_palette.hpp"
#include "vulkan/swapchain.hpp"

namespace rendering {
class BasicPipeline {
 public:
  explicit BasicPipeline(vulkan::Context& context, vulkan::Swapchain& swapchain,
                         vulkan::GlobalShaderData& globalData,
                         vulkan::MaterialPalette& materialPalette);
  ~BasicPipeline();

  void Bind(vk::CommandBuffer cmd);

  [[nodiscard]] vk::PipelineLayout GetLayout() const {
    return pipelineLayout_.get();
  }

 private:
  vulkan::Context& context_;
  vk::UniqueShaderModule vertShader_{VK_NULL_HANDLE};
  vk::UniqueShaderModule fragShader_{VK_NULL_HANDLE};
  std::unique_ptr<vulkan::DescriptorAllocator> descriptorAllocator_{nullptr};
  vk::UniqueDescriptorSetLayout descriptorSetLayout_{VK_NULL_HANDLE};
  vk::UniquePipelineLayout pipelineLayout_{VK_NULL_HANDLE};
  vk::DescriptorSet descriptorSet_{VK_NULL_HANDLE};
  vk::UniquePipeline pipeline_{VK_NULL_HANDLE};
};
}  // namespace rendering
