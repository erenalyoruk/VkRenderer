#pragma once

#include <memory>

#include "rhi/factory.hpp"

namespace backends::vulkan {
class VulkanContext;

class VulkanFactory : public rhi::Factory {
 public:
  explicit VulkanFactory(VulkanContext& context) : context_(context) {}

  std::unique_ptr<rhi::Buffer> CreateBuffer(rhi::Size size,
                                            rhi::BufferUsage usage,
                                            rhi::MemoryUsage memUsage) override;
  std::unique_ptr<rhi::Texture> CreateTexture(uint32_t width, uint32_t height,
                                              rhi::Format format,
                                              rhi::TextureUsage usage) override;
  std::unique_ptr<rhi::Sampler> CreateSampler(
      rhi::Filter magFilter, rhi::Filter minFilter,
      rhi::AddressMode addressMode) override;
  std::unique_ptr<rhi::Shader> CreateShader(
      rhi::ShaderStage stage, std::span<const uint32_t> spirv) override;
  std::unique_ptr<rhi::DescriptorSetLayout> CreateDescriptorSetLayout(
      std::span<const rhi::DescriptorBinding> bindings) override;
  std::unique_ptr<rhi::DescriptorSet> CreateDescriptorSet(
      const rhi::DescriptorSetLayout* layout) override;
  std::unique_ptr<rhi::PipelineLayout> CreatePipelineLayout(
      std::span<const rhi::DescriptorSetLayout* const> setLayouts,
      std::span<const rhi::PushConstantRange> pushConstantRanges = {}) override;
  std::unique_ptr<rhi::Pipeline> CreateGraphicsPipeline(
      const rhi::GraphicsPipelineDesc& desc) override;
  std::unique_ptr<rhi::CommandPool> CreateCommandPool(
      rhi::QueueType queueType = rhi::QueueType::Graphics) override;
  std::unique_ptr<rhi::Fence> CreateFence(bool signaled = false) override;
  std::unique_ptr<rhi::Semaphore> CreateSemaphore() override;
  std::unique_ptr<rhi::Swapchain> CreateSwapchain(uint32_t width,
                                                  uint32_t height,
                                                  rhi::Format format) override;

 private:
  VulkanContext& context_;  // NOLINT
};
}  // namespace backends::vulkan
