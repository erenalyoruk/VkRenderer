#pragma once

#include <entt/entt.hpp>

#include "renderer/bindless_materials.hpp"
#include "resource/types.hpp"

namespace resource {
/**
 * @brief Instantiate a loaded model into the ECS registry.
 *
 * @param registry The ECS registry to populate
 * @param model The loaded model data
 * @param bindlessMaterials Bindless material manager for registering
 * materials
 * @return Root entity of the instantiated scene
 */
entt::entity InstantiateModel(
    entt::registry& registry, const Model& model,
    renderer::BindlessMaterialManager& bindlessMaterials);

/**
 * @brief Instantiate a node and its children recursively.
 *
 * @param registry The ECS registry
 * @param model The loaded model data
 * @param materialIndices Material indices for the model
 * @param nodeIndex Index of the node to instantiate
 * @param parent Parent entity
 */
void InstantiateNode(entt::registry& registry, const Model& model,
                     const std::vector<uint32_t>& materialIndices,
                     uint32_t nodeIndex, entt::entity parent);
}  // namespace resource
