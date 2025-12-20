#pragma once

#include <memory>

#include "rhi/buffer.hpp"
#include "rhi/command.hpp"
#include "rhi/descriptor.hpp"
#include "rhi/pipeline.hpp"
#include "rhi/queue.hpp"
#include "rhi/sampler.hpp"
#include "rhi/swapchain.hpp"
#include "rhi/sync.hpp"
#include "rhi/texture.hpp"
#include "rhi/types.hpp"

namespace rhi {
/**
 * @brief Abstract factory interface for creating RHI resources.
 */
class Factory {
 public:
  virtual ~Factory() = default;

  /**
   * @brief Creates a buffer resource.
   *
   * @param size Size of the buffer to create
   * @param usage Buffer usage flags
   * @param memoryUsage Memory usage type
   * @return std::unique_ptr<Buffer> Pointer to the created buffer
   */
  virtual std::unique_ptr<Buffer> CreateBuffer(Size size, BufferUsage usage,
                                               MemoryUsage memoryUsage) = 0;

  /**
   * @brief Creates a texture resource.
   *
   * @param width Width of the texture
   * @param height Height of the texture
   * @param format Texture format
   * @param usage Texture usage flags
   * @return std::unique_ptr<Texture> Pointer to the created texture
   */
  virtual std::unique_ptr<Texture> CreateTexture(uint32_t width,
                                                 uint32_t height, Format format,
                                                 TextureUsage usage) = 0;

  /**
   * @brief Creates a sampler resource.
   *
   * @param magFilter Magnification filter.
   * @param minFilter Minification filter.
   * @param addressMode Texture address mode.
   * @return std::unique_ptr<Sampler> Pointer to the created sampler
   */
  virtual std::unique_ptr<Sampler> CreateSampler(Filter magFilter,
                                                 Filter minFilter,
                                                 AddressMode addressMode) = 0;

  /**
   * @brief Creates a shader resource.
   *
   * @param stage Shader stage (vertex, fragment, etc.)
   * @param spirv SPIR-V bytecode for the shader
   * @return std::unique_ptr<Shader> Pointer to the created shader
   */
  virtual std::unique_ptr<Shader> CreateShader(
      ShaderStage stage, std::span<const uint32_t> spirv) = 0;

  /**
   * @brief Creates a descriptor set layout.
   *
   * @param bindings Descriptor bindings for the layout
   * @return std::unique_ptr<DescriptorSetLayout> Pointer to the created
   * descriptor set layout
   */
  virtual std::unique_ptr<DescriptorSetLayout> CreateDescriptorSetLayout(
      std::span<const DescriptorBinding> bindings) = 0;

  /**
   * @brief Creates a descriptor set.
   *
   * @param layout Descriptor set layout
   * @return std::unique_ptr<DescriptorSet> Pointer to the created descriptor
   * set
   */
  virtual std::unique_ptr<DescriptorSet> CreateDescriptorSet(
      const DescriptorSetLayout* layout) = 0;

  /**
   * @brief Creates a pipeline layout.
   *
   * @param setLayouts Descriptor set layouts
   * @param pushConstantRanges Push constant ranges
   * @return std::unique_ptr<PipelineLayout> Pointer to the created pipeline
   * layout
   */
  virtual std::unique_ptr<PipelineLayout> CreatePipelineLayout(
      std::span<const DescriptorSetLayout* const> setLayouts,
      std::span<const PushConstantRange> pushConstantRanges = {}) = 0;

  /**
   * @brief Creates a graphics pipeline.
   *
   * @param desc Graphics pipeline description
   * @return std::unique_ptr<Pipeline> Pointer to the created graphics pipeline
   */
  virtual std::unique_ptr<Pipeline> CreateGraphicsPipeline(
      const GraphicsPipelineDesc& desc) = 0;

  /**
   * @brief Creates a compute pipeline.
   *
   * @param desc Compute pipeline description
   * @return std::unique_ptr<Pipeline> Pointer to the created compute pipeline
   */
  virtual std::unique_ptr<Pipeline> CreateComputePipeline(
      const ComputePipelineDesc& desc) = 0;

  /**
   * @brief Creates a command pool.
   *
   * @param queueType The type of queue this pool will submit to
   * @return std::unique_ptr<CommandPool> Pointer to the created command pool
   */
  virtual std::unique_ptr<CommandPool> CreateCommandPool(
      QueueType queueType = QueueType::Graphics) = 0;

  /**
   * @brief Creates a fence.
   *
   * @param signaled Whether the fence is initially signaled
   * @return std::unique_ptr<Fence> Pointer to the created fence
   */
  virtual std::unique_ptr<Fence> CreateFence(bool signaled = false) = 0;

  /**
   * @brief Creates a semaphore.
   *
   * @return std::unique_ptr<Semaphore> Pointer to the created semaphore
   */
  virtual std::unique_ptr<Semaphore> CreateSemaphore() = 0;

  /**
   * @brief Creates a swapchain.
   *
   * @param width Width of the swapchain images
   * @param height Height of the swapchain images
   * @param format Format of the swapchain images
   * @return std::unique_ptr<Swapchain> Pointer to the created swapchain
   */
  virtual std::unique_ptr<Swapchain> CreateSwapchain(uint32_t width,
                                                     uint32_t height,
                                                     Format format) = 0;

  /**
   * @brief Creates a cubemap texture resource.
   *
   * @param size Size of each cubemap face (width = height)
   * @param format Texture format
   * @param usage Texture usage flags
   * @param mipLevels Number of mip levels (for prefiltered maps)
   * @return std::unique_ptr<Texture> Pointer to the created cubemap
   */
  virtual std::unique_ptr<Texture> CreateCubemap(uint32_t size, Format format,
                                                 TextureUsage usage,
                                                 uint32_t mipLevels = 1) = 0;
};
}  // namespace rhi
