#pragma once

#include <array>
#include <cstdint>
#include <span>

#include "rhi/buffer.hpp"
#include "rhi/descriptor.hpp"
#include "rhi/pipeline.hpp"
#include "rhi/texture.hpp"

namespace rhi {

/**
 * @brief Load operation for attachments
 */
enum class LoadOp : uint8_t {
  Load,      // Load previous contents
  Clear,     // Clear to a value
  DontCare,  // Don't care about previous contents
};

/**
 * @brief Store operation for attachments
 */
enum class StoreOp : uint8_t {
  Store,     // Store the results
  DontCare,  // Don't care about storing
};

/**
 * @brief Describes a color attachment for rendering
 */
struct RenderingAttachment {
  Texture* texture{nullptr};
  ImageLayout layout{ImageLayout::ColorAttachment};
  LoadOp loadOp{LoadOp::Clear};
  StoreOp storeOp{StoreOp::Store};
  std::array<float, 4> clearValue{0.0F, 0.0F, 0.0F, 1.0F};
};

/**
 * @brief Describes the rendering area and attachments
 */
struct RenderingInfo {
  uint32_t width{0};
  uint32_t height{0};
  std::span<const RenderingAttachment> colorAttachments;
  const RenderingAttachment* depthAttachment{nullptr};
};

/**
 * @brief Abstract base class for command buffers in the rendering hardware
 * interface (RHI).
 */
class CommandBuffer {
 public:
  virtual ~CommandBuffer() = default;

  /**
   * @brief Begins recording commands into the command buffer.
   */
  virtual void Begin() = 0;

  /**
   * @brief Ends recording commands into the command buffer.
   */
  virtual void End() = 0;

  /**
   * @brief Begins a dynamic rendering pass.
   *
   * @param info Rendering info describing attachments and area.
   */
  virtual void BeginRendering(const RenderingInfo& info) = 0;

  /**
   * @brief Ends the current dynamic rendering pass.
   */
  virtual void EndRendering() = 0;

  /**
   * @brief Sets the viewport dynamically.
   *
   * @param x X coordinate of the viewport.
   * @param y Y coordinate of the viewport.
   * @param width Width of the viewport.
   * @param height Height of the viewport.
   * @param minDepth Minimum depth of the viewport.
   * @param maxDepth Maximum depth of the viewport.
   */
  virtual void SetViewport(float x, float y, float width, float height,
                           float minDepth = 0.0F, float maxDepth = 1.0F) = 0;

  /**
   * @brief Sets the scissor rect dynamically.
   *
   * @param x X coordinate of the scissor rect.
   * @param y Y coordinate of the scissor rect.
   * @param width Width of the scissor rect.
   * @param height Height of the scissor rect.
   */
  virtual void SetScissor(int32_t x, int32_t y, uint32_t width,
                          uint32_t height) = 0;

  /**
   * @brief Binds pipeline state to the command buffer.
   *
   * @param pipeline The pipeline state to bind.
   */
  virtual void BindPipeline(const Pipeline* pipeline) = 0;

  /**
   * @brief Binds descriptor sets to the command buffer.
   *
   * @param pipeline The pipeline associated with the descriptor sets.
   * @param firstSet The index of the first descriptor set.
   * @param sets The descriptor sets to bind.
   */
  virtual void BindDescriptorSets(
      const Pipeline* pipeline, uint32_t firstSet,
      std::span<const DescriptorSet* const> sets) = 0;

  /**
   * @brief Binds vertex buffers to the command buffer.
   *
   * @param firstBinding The index of the first binding.
   * @param buffers The vertex buffers to bind.
   * @param offsets The offsets within each buffer.
   */
  virtual void BindVertexBuffers(uint32_t firstBinding,
                                 std::span<const Buffer* const> buffers,
                                 std::span<const uint64_t> offsets) = 0;

  /**
   * @brief Binds an index buffer to the command buffer.
   *
   * @param buffer The index buffer to bind.
   * @param offset The offset within the buffer.
   * @param is32Bit Indicates whether the index buffer uses 32-bit indices.
   */
  virtual void BindIndexBuffer(const Buffer& buffer, uint64_t offset,
                               bool is32Bit) = 0;

  /**
   * @brief Issues a draw command.
   *
   * @param vertexCount Vertex count to draw.
   * @param instanceCount Instance count to draw.
   * @param firstVertex First vertex to draw.
   * @param firstInstance First instance to draw.
   */
  virtual void Draw(uint32_t vertexCount, uint32_t instanceCount,
                    uint32_t firstVertex, uint32_t firstInstance) = 0;

  /**
   * @brief Issues an indexed draw command.
   *
   * @param indexCount Index count to draw.
   * @param instanceCount Instance count to draw.
   * @param firstIndex First index to draw.
   * @param vertexOffset Vertex offset to add to each index.
   * @param firstInstance First instance to draw.
   */
  virtual void DrawIndexed(uint32_t indexCount, uint32_t instanceCount,
                           uint32_t firstIndex, int32_t vertexOffset,
                           uint32_t firstInstance) = 0;

  /**
   * @brief Transitions a texture to a new layout.
   *
   * @param texture The texture to transition.
   * @param oldLayout The current layout of the texture.
   * @param newLayout The target layout of the texture.
   */
  virtual void TransitionTexture(Texture* texture, ImageLayout oldLayout,
                                 ImageLayout newLayout) = 0;

  /**
   * @brief Copies data from one buffer to another.
   *
   * @param src The source buffer.
   * @param dst The destination buffer.
   * @param srcOffset Offset in the source buffer.
   * @param dstOffset Offset in the destination buffer.
   * @param size Number of bytes to copy.
   */
  virtual void CopyBuffer(const Buffer* src, Buffer* dst, Size srcOffset,
                          Size dstOffset, Size size) = 0;

  /**
   * @brief Copies data from a buffer to a texture.
   *
   * @param src The source buffer.
   * @param dst The destination texture.
   * @param mipLevel The mip level of the texture to copy to.
   * @param arrayLayer The array layer of the texture to copy to.
   */
  virtual void CopyBufferToTexture(const Buffer* src, Texture* dst,
                                   uint32_t mipLevel = 0,
                                   uint32_t arrayLayer = 0) = 0;

  /**
   * @brief Pushes constants to the pipeline.
   *
   * @param pipeline The pipeline to push constants to.
   * @param offset Offset in bytes.
   * @param data The constant data.
   */
  virtual void PushConstants(const Pipeline* pipeline, uint32_t offset,
                             std::span<const std::byte> data) = 0;
};

/**
 * @brief Abstract base class for command pools in the rendering hardware
 * interface (RHI).
 */
class CommandPool {
 public:
  virtual ~CommandPool() = default;

  /**
   * @brief Resets the command pool, freeing all allocated command buffers.
   */
  virtual void Reset() = 0;

  /**
   * @brief Allocates a command buffer from the command pool.
   *
   * @return A pointer to the allocated command buffer.
   */
  [[nodiscard]] virtual CommandBuffer* AllocateCommandBuffer() = 0;
};
}  // namespace rhi
