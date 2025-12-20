#pragma once

#include <memory>
#include <vector>

#include <vulkan/vulkan.hpp>

#include "rhi/command.hpp"
#include "rhi/queue.hpp"

namespace backends::vulkan {
class VulkanContext;

class VulkanCommandBuffer : public rhi::CommandBuffer {
 public:
  explicit VulkanCommandBuffer(vk::CommandBuffer commandBuffer,
                               vk::Device device);
  ~VulkanCommandBuffer() override = default;

  void Begin() override;
  void End() override;

  void BeginRendering(const rhi::RenderingInfo& info) override;

  void EndRendering() override;

  void SetViewport(float x, float y, float width, float height, float minDepth,
                   float maxDepth) override;

  void SetScissor(int32_t x, int32_t y, uint32_t width,
                  uint32_t height) override;

  void BindPipeline(const rhi::Pipeline* pipeline) override;

  void BindDescriptorSets(
      const rhi::Pipeline* pipeline, uint32_t firstSet,
      std::span<const rhi::DescriptorSet* const> sets) override;

  void BindVertexBuffers(uint32_t firstBinding,
                         std::span<const rhi::Buffer* const> buffers,
                         std::span<const uint64_t> offsets) override;

  void BindIndexBuffer(const rhi::Buffer& buffer, uint64_t offset,
                       bool is32Bit) override;

  void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex,
            uint32_t firstInstance) override;

  void DrawIndexed(uint32_t indexCount, uint32_t instanceCount,
                   uint32_t firstIndex, int32_t vertexOffset,
                   uint32_t firstInstance) override;

  void DrawIndexedIndirect(const rhi::Buffer* buffer, rhi::Size offset,
                           uint32_t drawCount, uint32_t stride) override;

  void DrawIndexedIndirectCount(const rhi::Buffer* commandBuffer,
                                rhi::Size commandOffset,
                                const rhi::Buffer* countBuffer,
                                rhi::Size countOffset, uint32_t maxDrawCount,
                                uint32_t stride) override;

  void Dispatch(uint32_t groupCountX, uint32_t groupCountY,
                uint32_t groupCountZ) override;

  void BufferBarrier(const rhi::Buffer* buffer, rhi::AccessFlags srcAccess,
                     rhi::AccessFlags dstAccess) override;

  void FillBuffer(rhi::Buffer* buffer, rhi::Size offset, rhi::Size size,
                  uint32_t value) override;

  void TransitionTexture(rhi::Texture* texture, rhi::ImageLayout oldLayout,
                         rhi::ImageLayout newLayout) override;

  void CopyBuffer(const rhi::Buffer* src, rhi::Buffer* dst, rhi::Size srcOffset,
                  rhi::Size dstOffset, rhi::Size size) override;

  void CopyBufferToTexture(const rhi::Buffer* src, rhi::Texture* dst,
                           uint32_t mipLevel, uint32_t arrayLayer) override;

  void PushConstants(const rhi::Pipeline* pipeline, uint32_t offset,
                     std::span<const std::byte> data) override;

  [[nodiscard]] vk::CommandBuffer GetCommandBuffer() const {
    return commandBuffer_;
  }

 private:
  vk::CommandBuffer commandBuffer_;
  vk::Device device_;
};

class VulkanCommandPool : public rhi::CommandPool {
 public:
  static std::unique_ptr<VulkanCommandPool> Create(VulkanContext& context,
                                                   rhi::QueueType queueType);

  void Reset() override;
  rhi::CommandBuffer* AllocateCommandBuffer() override;

 private:
  VulkanCommandPool(vk::UniqueCommandPool commandPool, vk::Device device);

  vk::UniqueCommandPool commandPool_;
  vk::Device device_;
  std::vector<std::unique_ptr<VulkanCommandBuffer>> allocatedBuffers_;
};
}  // namespace backends::vulkan
