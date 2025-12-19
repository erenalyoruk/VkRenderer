#pragma once

#include <cstdint>
#include <span>

#include "rhi/buffer.hpp"
#include "rhi/descriptor.hpp"
#include "rhi/pipeline.hpp"
#include "rhi/swapchain.hpp"
#include "rhi/sync.hpp"
#include "rhi/texture.hpp"

namespace rhi {
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
   * @brief Binds pipeline state to the command buffer.
   *
   * @param pipeline The pipeline state to bind.
   */
  virtual void BindPipeline(const Pipeline& pipeline) = 0;

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
   * TODO: Add old/new layouts as parameters.
   */
  virtual void TransitionTexture(Texture* texture /*, old/new layouts*/) = 0;
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

/**
 * @brief Abstract base class for command queues in the rendering hardware
 * interface (RHI).
 */
class CommandQueue {
 public:
  virtual ~CommandQueue() = default;

  /**
   * @brief Submits command buffers to the queue for execution.
   *
   * @param commandBuffers The command buffers to submit.
   * @param waitSemaphores Semaphores to wait on before execution.
   * @param signalSemaphores Semaphores to signal after execution.
   * @param fence Optional fence to signal upon completion.
   */
  virtual void Submit(std::span<CommandBuffer* const> commandBuffers,
                      std::span<Semaphore* const> waitSemaphores,
                      std::span<Semaphore* const> signalSemaphores,
                      Fence* fence = nullptr) = 0;

  /**
   * @brief Presents the rendered image to the display.
   *
   * @param swapchain The swapchain to present to.
   * @param imageIndex The index of the image to present.
   */
  virtual void Present(Swapchain* swapchain, uint32_t imageIndex,
                       std::span<Semaphore* const> waitSemaphores) = 0;
};
}  // namespace rhi
