#include "resource/scene_loader.hpp"

#include "ecs/components.hpp"

namespace resource {

entt::entity InstantiateModel(
    entt::registry& registry, const Model& model,
    renderer::BindlessMaterialManager& bindlessMaterials) {
  // Register all materials and get their bindless indices
  std::vector<uint32_t> materialIndices;
  materialIndices.reserve(model.materials.size());

  for (const auto& material : model.materials) {
    uint32_t idx = bindlessMaterials.RegisterMaterial(material, model.textures);
    materialIndices.push_back(idx);
  }

  // If no materials, use default (index 0)
  if (materialIndices.empty()) {
    materialIndices.push_back(0);  // Default material is always index 0
  }

  // Create root entity
  entt::entity root = registry.create();
  registry.emplace<ecs::TransformComponent>(root);
  registry.emplace<ecs::WorldTransformComponent>(root);

  // Instantiate all root nodes
  for (uint32_t nodeIndex : model.rootNodes) {
    InstantiateNode(registry, model, materialIndices, nodeIndex, root);
  }

  return root;
}

// NOLINTNEXTLINE
void InstantiateNode(entt::registry& registry, const Model& model,
                     const std::vector<uint32_t>& materialIndices,
                     uint32_t nodeIndex, entt::entity parent) {
  if (nodeIndex >= model.nodes.size()) {
    return;
  }

  const auto& node = model.nodes[nodeIndex];

  entt::entity entity = registry.create();

  // Transform
  auto& transform = registry.emplace<ecs::TransformComponent>(entity);
  transform.position = node.translation;
  transform.rotation = node.rotation;
  transform.scale = node.scale;

  registry.emplace<ecs::WorldTransformComponent>(entity);

  // Hierarchy
  auto& hierarchy = registry.emplace<ecs::HierarchyComponent>(entity);
  hierarchy.parent = parent;

  if (parent != entt::null &&
      registry.all_of<ecs::HierarchyComponent>(parent)) {
    auto& parentHierarchy = registry.get<ecs::HierarchyComponent>(parent);
    parentHierarchy.children.push_back(entity);
  }

  // Mesh
  if (node.meshIndex >= 0 &&
      node.meshIndex < static_cast<int32_t>(model.meshes.size())) {
    const auto& mesh = model.meshes[node.meshIndex];

    auto& meshComp = registry.emplace<ecs::MeshComponent>(entity);
    meshComp.vertexBuffer = mesh.vertexBuffer;
    meshComp.indexBuffer = mesh.indexBuffer;

    for (const auto& prim : mesh.primitives) {
      ecs::SubMesh subMesh{};
      subMesh.indexCount = prim.indexCount;
      subMesh.indexOffset = prim.indexOffset;
      subMesh.vertexOffset = prim.vertexOffset;

      // Map to bindless material index
      if (prim.materialIndex >= 0 &&
          prim.materialIndex < static_cast<int32_t>(materialIndices.size())) {
        subMesh.materialIndex = materialIndices[prim.materialIndex];
      } else {
        subMesh.materialIndex = 0;  // Default material
      }

      meshComp.subMeshes.push_back(subMesh);
    }

    // Material component with bindless indices
    auto& matComp = registry.emplace<ecs::MaterialComponent>(entity);
    matComp.materialIndices = materialIndices;

    // Bounding box
    registry.emplace<ecs::BoundingBoxComponent>(entity, mesh.bounds);

    // Renderable tag
    registry.emplace<ecs::RenderableComponent>(entity);
  }

  // Recursively instantiate children
  for (uint32_t childIndex : node.children) {
    InstantiateNode(registry, model, materialIndices, childIndex, entity);
  }
}

}  // namespace resource
