#pragma once

#include <entt/entt.hpp>

#include "ecs/components.hpp"
#include "render_context.hpp"
#include "rhi/device.hpp"

namespace renderer {

struct RenderStats {
  uint32_t entitiesProcessed{0};
  uint32_t drawCalls{0};
  uint32_t triangles{0};
  uint32_t culledObjects{0};
};

class RenderSystem {
 public:
  RenderSystem(rhi::Device& device, rhi::Factory& factory);

  void Render(entt::registry& registry, float deltaTime);

  [[nodiscard]] const RenderStats& GetStats() const { return stats_; }

 private:
  void UpdateTransforms(entt::registry& registry);
  void FrustumCull(entt::registry& registry);
  void SortRenderables(entt::registry& registry);
  void ExecuteRendering(entt::registry& registry, uint32_t imageIndex);

  rhi::Device& device_;
  RenderContext context_;
  RenderStats stats_;
  uint32_t frameCounter_{0};

  // Cached camera data
  ecs::CameraComponent* activeCamera_{nullptr};
};

}  // namespace renderer
