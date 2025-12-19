#pragma once

#include <entt/entt.hpp>
#include <vulkan/vulkan.hpp>

#include "rendering/pipelines/basic_pipeline.hpp"

namespace rendering {
class RenderingSystem {
 public:
  void Render(entt::registry& registry, vk::CommandBuffer cmd,
              BasicPipeline& pipeline);
};
}  // namespace rendering
