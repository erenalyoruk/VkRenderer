#pragma once

#include <memory>
#include <array>

#include <glm/glm.hpp>

#include "rhi/buffer.hpp"
#include "rhi/command.hpp"
#include "rhi/descriptor.hpp"
#include "rhi/device.hpp"
#include "rhi/factory.hpp"
#include "rhi/pipeline.hpp"

namespace renderer {

// Must match shader struct - object instance data
struct alignas(16) ObjectData {
  glm::mat4 model;
  glm::mat4 normalMatrix;
  glm::vec4 boundingSphere;  // xyz = center (local space), w = radius
  uint32_t materialIndex;
  uint32_t indexCount;
  uint32_t indexOffset;
  int32_t vertexOffset;
};

// VkDrawIndexedIndirectCommand compatible
struct DrawIndexedIndirectCommand {
  uint32_t indexCount;
  uint32_t instanceCount;
  uint32_t firstIndex;
  int32_t vertexOffset;
  uint32_t firstInstance;  // Used as object index to fetch transform
};

struct CullUniforms {
  glm::mat4 viewProjection;
  std::array<glm::vec4, 6> frustumPlanes;
  uint32_t objectCount;
  uint32_t _padding[3];  // NOLINT
};

class GPUCulling {
 public:
  GPUCulling(rhi::Factory& factory, rhi::Device& device);

  void Initialize();

  // Update object data for culling (call when scene changes)
  void UpdateObjects(std::span<const ObjectData> objects);

  // Update camera frustum
  void UpdateFrustum(const glm::mat4& viewProjection);

  // Reset draw count to zero (call before culling)
  void ResetDrawCount(rhi::CommandBuffer* cmd);

  // Execute culling compute pass
  void Execute(rhi::CommandBuffer* cmd);

  // Get buffers for rendering
  [[nodiscard]] rhi::Buffer* GetObjectBuffer() const {
    return objectBuffer_.get();
  }
  [[nodiscard]] rhi::Buffer* GetDrawCommandBuffer() const {
    return drawCommandBuffer_.get();
  }
  [[nodiscard]] rhi::Buffer* GetDrawCountBuffer() const {
    return drawCountBuffer_.get();
  }
  [[nodiscard]] uint32_t GetMaxDrawCount() const { return maxObjects_; }
  [[nodiscard]] uint32_t GetObjectCount() const { return objectCount_; }

  // Get descriptor layout for object data (for graphics pipeline)
  [[nodiscard]] rhi::DescriptorSetLayout* GetObjectDescriptorLayout() const {
    return objectDescriptorLayout_.get();
  }
  [[nodiscard]] rhi::DescriptorSet* GetObjectDescriptorSet() const {
    return objectDescriptorSet_.get();
  }

 private:
  void CreateBuffers();
  void CreatePipeline();
  void ExtractFrustumPlanes(const glm::mat4& viewProj, glm::vec4* planes);

  rhi::Factory& factory_;
  rhi::Device& device_;

  // Culling pipeline
  std::unique_ptr<rhi::Shader> cullShader_;
  std::unique_ptr<rhi::DescriptorSetLayout> cullDescriptorLayout_;
  std::unique_ptr<rhi::PipelineLayout> cullPipelineLayout_;
  std::unique_ptr<rhi::Pipeline> cullPipeline_;
  std::unique_ptr<rhi::DescriptorSet> cullDescriptorSet_;

  // Object data descriptor for graphics pipeline (set 2)
  std::unique_ptr<rhi::DescriptorSetLayout> objectDescriptorLayout_;
  std::unique_ptr<rhi::DescriptorSet> objectDescriptorSet_;

  // Buffers
  std::unique_ptr<rhi::Buffer> objectBuffer_;  // Object transforms + bounds
  std::unique_ptr<rhi::Buffer> cullUniformBuffer_;  // Frustum planes
  std::unique_ptr<rhi::Buffer> drawCommandBuffer_;  // Indirect commands
  std::unique_ptr<rhi::Buffer> drawCountBuffer_;    // Visible count

  uint32_t maxObjects_{10000};
  uint32_t objectCount_{0};
};

}  // namespace renderer
