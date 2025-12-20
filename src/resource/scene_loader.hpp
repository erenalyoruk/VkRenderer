#pragma once

#include <entt/entt.hpp>

#include "resource/types.hpp"

namespace resource {
class SceneLoader {
 public:
  /**
   * @brief Instantiate a model into the ECS registry.
   * @param registry The ECS registry
   * @param model The loaded model
   * @param rootTransform Optional root transform for the model
   * @return Root entity of the instantiated scene
   */
  static entt::entity Instantiate(
      entt::registry& registry, const Model& model,
      const ecs::TransformComponent& rootTransform = {});

 private:
  static entt::entity InstantiateNode(entt::registry& registry,
                                      const Model& model, uint32_t nodeIndex,
                                      entt::entity parent);
};
}  // namespace resource
