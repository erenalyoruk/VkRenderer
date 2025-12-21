#pragma once

#include <cstdint>
#include <memory>
#include <span>

#include <glm/glm.hpp>

#include "rhi/buffer.hpp"
#include "rhi/command.hpp"
#include "rhi/descriptor.hpp"
#include "rhi/device.hpp"
#include "rhi/factory.hpp"
#include "rhi/pipeline.hpp"

namespace renderer {

// GPU Light types
enum class LightType : uint8_t { Point = 0, Spot = 1 };

// GPU-friendly light structure (matches shader)
struct alignas(16) GPULight {
  glm::vec4 positionAndRadius;  // xyz = position, w = radius
  glm::vec4 colorAndIntensity;  // xyz = color, w = intensity
  glm::vec4 directionAndType;   // xyz = direction (spot), w = type
  glm::vec4 spotParams;         // x = cos(inner), y = cos(outer), zw = unused
};

// Tile configuration
constexpr uint32_t kTileSize = 16;
constexpr uint32_t kMaxLightsPerTile = 256;
constexpr uint32_t kMaxLights = 1024;

// Light culling uniforms
struct alignas(16) LightCullUniforms {
  glm::mat4 view;
  glm::mat4 projection;
  glm::mat4 invProjection;
  glm::uvec4 screenDimensions;  // x = width, y = height, z = tileCountX, w =
                                // tileCountY
  uint32_t lightCount;
  float nearPlane;
  float farPlane;
  uint32_t _padding;  // NOLINT
};

class ForwardPlus {
 public:
  ForwardPlus(rhi::Factory& factory, rhi::Device& device);

  void Initialize();
  void Shutdown();

  // Update lights from scene
  void UpdateLights(std::span<const GPULight> lights);

  // Update screen dimensions (call on resize)
  void UpdateScreenSize(uint32_t width, uint32_t height);

  // Update camera matrices for culling
  void UpdateCamera(const glm::mat4& view, const glm::mat4& projection,
                    float nearPlane, float farPlane);

  // Execute light culling compute pass
  void ExecuteLightCulling(rhi::CommandBuffer* cmd);

  // Get descriptor layout for light data (for graphics pipeline)
  [[nodiscard]] rhi::DescriptorSetLayout* GetLightDescriptorLayout() const {
    return lightDescriptorLayout_.get();
  }

  [[nodiscard]] rhi::DescriptorSet* GetLightDescriptorSet() const {
    return lightDescriptorSet_.get();
  }

  [[nodiscard]] uint32_t GetLightCount() const { return lightCount_; }
  [[nodiscard]] glm::uvec2 GetTileCount() const { return tileCount_; }

 private:
  void CreateBuffers();
  void CreatePipeline();
  void UpdateTileCount();

  rhi::Factory& factory_;
  rhi::Device& device_;

  // Screen/tile dimensions
  uint32_t screenWidth_{1920};
  uint32_t screenHeight_{1080};
  glm::uvec2 tileCount_{0};

  // Light data
  uint32_t lightCount_{0};

  // Buffers
  std::unique_ptr<rhi::Buffer> lightBuffer_;  // GPULight array
  std::unique_ptr<rhi::Buffer> lightCullUniformBuffer_;
  std::unique_ptr<rhi::Buffer> lightIndexBuffer_;  // Per-tile light indices
  std::unique_ptr<rhi::Buffer> lightGridBuffer_;   // Per-tile offset/count

  // Light culling pipeline
  std::unique_ptr<rhi::Shader> lightCullShader_;
  std::unique_ptr<rhi::DescriptorSetLayout> cullDescriptorLayout_;
  std::unique_ptr<rhi::PipelineLayout> cullPipelineLayout_;
  std::unique_ptr<rhi::Pipeline> cullPipeline_;
  std::unique_ptr<rhi::DescriptorSet> cullDescriptorSet_;

  // Light descriptor for graphics pipeline (set 4)
  std::unique_ptr<rhi::DescriptorSetLayout> lightDescriptorLayout_;
  std::unique_ptr<rhi::DescriptorSet> lightDescriptorSet_;

  // Camera data cache
  LightCullUniforms cullUniforms_{};
};

}  // namespace renderer
