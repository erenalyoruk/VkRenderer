#pragma once

#include <entt/entt.hpp>

#include "ecs/components.hpp"
#include "renderer/forward_plus.hpp"
#include "renderer/gpu_culling.hpp"
#include "renderer/render_context.hpp"
#include "rhi/device.hpp"

namespace renderer {

class RenderSystem {
 public:
  RenderSystem(rhi::Device& device, rhi::Factory& factory);

  void Render(entt::registry& registry, float deltaTime);

  void OnSwapchainResized() { context_.OnSwapchainResized(); }

  void SetActivePipeline(PipelineType type) { activePipeline_ = type; }
  [[nodiscard]] PipelineType GetActivePipeline() const {
    return activePipeline_;
  }

  [[nodiscard]] RenderContext& GetContext() { return context_; }

 private:
  void UpdateTransforms(entt::registry& registry);
  void BuildObjectDataForCulling(entt::registry& registry);
  void CollectLights(entt::registry& registry);
  void ExecuteGPUDrivenRendering(entt::registry& registry, uint32_t imageIndex);

  rhi::Device& device_;
  rhi::Factory& factory_;
  RenderContext context_;
  uint32_t frameCounter_{0};
  float totalTime_{0.0F};

  PipelineType activePipeline_{PipelineType::PBRLit};
  ecs::CameraComponent* activeCamera_{nullptr};

  std::vector<ObjectData> objectDataCache_;
  std::vector<GPULight> lightCache_;

  // Camera parameters for Forward+
  float cameraNear_{0.1F};
  float cameraFar_{1000.0F};
};

}  // namespace renderer
