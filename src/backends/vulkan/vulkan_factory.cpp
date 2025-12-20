#include "backends/vulkan/vulkan_factory.hpp"

#include "backends/vulkan/vulkan_buffer.hpp"
#include "backends/vulkan/vulkan_command.hpp"
#include "backends/vulkan/vulkan_context.hpp"
#include "backends/vulkan/vulkan_descriptor.hpp"
#include "backends/vulkan/vulkan_fence.hpp"
#include "backends/vulkan/vulkan_pipeline.hpp"
#include "backends/vulkan/vulkan_sampler.hpp"
#include "backends/vulkan/vulkan_semaphore.hpp"
#include "backends/vulkan/vulkan_shader.hpp"
#include "backends/vulkan/vulkan_swapchain.hpp"
#include "backends/vulkan/vulkan_texture.hpp"

namespace backends::vulkan {
std::unique_ptr<rhi::Buffer> VulkanFactory::CreateBuffer(
    rhi::Size size, rhi::BufferUsage usage, rhi::MemoryUsage memUsage) {
  return VulkanBuffer::Create(context_.GetAllocator(), size, usage, memUsage);
}

std::unique_ptr<rhi::Texture> VulkanFactory::CreateTexture(
    uint32_t width, uint32_t height, rhi::Format format,
    rhi::TextureUsage usage) {
  return VulkanTexture::Create(context_, width, height, format, usage);
}

std::unique_ptr<rhi::Sampler> VulkanFactory::CreateSampler(
    rhi::Filter magFilter, rhi::Filter minFilter,
    rhi::AddressMode addressMode) {
  return VulkanSampler::Create(context_, magFilter, minFilter, addressMode,
                               addressMode);
}

std::unique_ptr<rhi::Shader> VulkanFactory::CreateShader(
    rhi::ShaderStage stage, std::span<const uint32_t> spirv) {
  return VulkanShader::Create(context_, stage, spirv);
}

std::unique_ptr<rhi::DescriptorSetLayout>
VulkanFactory::CreateDescriptorSetLayout(
    std::span<const rhi::DescriptorBinding> bindings) {
  return VulkanDescriptorSetLayout::Create(context_, bindings);
}

std::unique_ptr<rhi::DescriptorSet> VulkanFactory::CreateDescriptorSet(
    const rhi::DescriptorSetLayout* layout) {
  const auto* vkLayout{std::bit_cast<const VulkanDescriptorSetLayout*>(layout)};
  return VulkanDescriptorSet::Create(context_, vkLayout);
}

std::unique_ptr<rhi::PipelineLayout> VulkanFactory::CreatePipelineLayout(
    std::span<const rhi::DescriptorSetLayout* const> setLayouts,
    std::span<const rhi::PushConstantRange> pushConstantRanges) {
  return VulkanPipelineLayout::Create(context_, setLayouts, pushConstantRanges);
}

std::unique_ptr<rhi::Pipeline> VulkanFactory::CreateGraphicsPipeline(
    const rhi::GraphicsPipelineDesc& desc) {
  return VulkanPipeline::Create(context_, desc);
}

std::unique_ptr<rhi::CommandPool> VulkanFactory::CreateCommandPool(
    rhi::QueueType queueType) {
  return VulkanCommandPool::Create(context_, queueType);
}

std::unique_ptr<rhi::Fence> VulkanFactory::CreateFence(bool signaled) {
  return VulkanFence::Create(context_, signaled);
}

std::unique_ptr<rhi::Semaphore> VulkanFactory::CreateSemaphore() {
  return VulkanSemaphore::Create(context_);
}

std::unique_ptr<rhi::Swapchain> VulkanFactory::CreateSwapchain(
    uint32_t width, uint32_t height, rhi::Format format) {
  return VulkanSwapchain::Create(context_, width, height, format);
}
}  // namespace backends::vulkan
