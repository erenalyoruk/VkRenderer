#include "backends/vulkan/vulkan_factory.hpp"

#include "backends/vulkan/vulkan_buffer.hpp"
#include "backends/vulkan/vulkan_context.hpp"

namespace backends::vulkan {
std::unique_ptr<rhi::Buffer> VulkanFactory::CreateBuffer(
    rhi::Size size, rhi::BufferUsage usage, rhi::MemoryUsage memUsage) {
  return VulkanBuffer::Create(context_.GetAllocator(), size, usage, memUsage);
}

// Placeholder implementations (return nullptr for now; implement as we go)
std::unique_ptr<rhi::Texture> VulkanFactory::CreateTexture(
    uint32_t width, uint32_t height, rhi::Format format,
    rhi::TextureUsage usage) {
  // TODO: Implement VulkanTexture
  return nullptr;
}

std::unique_ptr<rhi::Sampler> VulkanFactory::CreateSampler(
    rhi::Filter magFilter, rhi::Filter minFilter,
    rhi::AddressMode addressMode) {
  // TODO: Implement VulkanSampler
  return nullptr;
}

std::unique_ptr<rhi::Shader> VulkanFactory::CreateShader(
    rhi::ShaderStage stage, std::span<const uint32_t> spirv) {
  // TODO: Implement VulkanShader
  return nullptr;
}

std::unique_ptr<rhi::DescriptorSetLayout>
VulkanFactory::CreateDescriptorSetLayout(
    std::span<const rhi::DescriptorBinding> bindings) {
  // TODO: Implement VulkanDescriptorSetLayout
  return nullptr;
}

std::unique_ptr<rhi::DescriptorSet> VulkanFactory::CreateDescriptorSet(
    const rhi::DescriptorSetLayout* layout) {
  // TODO: Implement VulkanDescriptorSet
  return nullptr;
}

std::unique_ptr<rhi::PipelineLayout> VulkanFactory::CreatePipelineLayout(
    std::span<const rhi::DescriptorSetLayout* const> setLayouts) {
  // TODO: Implement VulkanPipelineLayout
  return nullptr;
}

std::unique_ptr<rhi::Pipeline> VulkanFactory::CreateGraphicsPipeline(
    const rhi::GraphicsPipelineDesc& desc) {
  // TODO: Implement VulkanPipeline
  return nullptr;
}

std::unique_ptr<rhi::CommandPool> VulkanFactory::CreateCommandPool() {
  // TODO: Implement VulkanCommandPool
  return nullptr;
}

std::unique_ptr<rhi::Fence> VulkanFactory::CreateFence(bool signaled) {
  // TODO: Implement VulkanFence
  return nullptr;
}

std::unique_ptr<rhi::Semaphore> VulkanFactory::CreateSemaphore() {
  // TODO: Implement VulkanSemaphore
  return nullptr;
}

std::unique_ptr<rhi::Swapchain> VulkanFactory::CreateSwapchain(
    uint32_t width, uint32_t height, rhi::Format format) {
  // TODO: Implement VulkanSwapchain
  return nullptr;
}
}  // namespace backends::vulkan
