#include "resource/scene_loader.hpp"

namespace resource {

entt::entity SceneLoader::Instantiate(
    entt::registry& registry, const Model& model,
    renderer::MaterialManager& materialManager,
    const ecs::TransformComponent& rootTransform) {
  // Create GPU materials for all model materials
  std::vector<renderer::GPUMaterial*> gpuMaterials;
  gpuMaterials.reserve(model.materials.size());

  for (const auto& material : model.materials) {
    auto* gpuMat = materialManager.CreateMaterial(material, model.textures);
    gpuMaterials.push_back(gpuMat);
  }

  // If no materials, add default
  if (gpuMaterials.empty()) {
    gpuMaterials.push_back(materialManager.GetDefaultMaterial());
  }

  // Create root entity
  auto root = registry.create();
  registry.emplace<ecs::TransformComponent>(root, rootTransform);
  registry.emplace<ecs::WorldTransformComponent>(root);

  // Instantiate all root nodes as children of root
  for (uint32_t nodeIndex : model.rootNodes) {
    InstantiateNode(registry, model, gpuMaterials, nodeIndex, root);
  }

  return root;
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity, misc-no-recursion)
entt::entity SceneLoader::InstantiateNode(
    entt::registry& registry, const Model& model,
    const std::vector<renderer::GPUMaterial*>& gpuMaterials, uint32_t nodeIndex,
    entt::entity parent) {
  const auto& node = model.nodes[nodeIndex];

  auto entity = registry.create();

  // Transform
  ecs::TransformComponent transform;
  transform.position = node.translation;
  transform.rotation = node.rotation;
  transform.scale = node.scale;
  registry.emplace<ecs::TransformComponent>(entity, transform);
  registry.emplace<ecs::WorldTransformComponent>(entity);

  // Hierarchy
  if (parent != entt::null) {
    auto& parentHierarchy =
        registry.get_or_emplace<ecs::HierarchyComponent>(parent);
    parentHierarchy.children.push_back(entity);

    auto& hierarchy = registry.emplace<ecs::HierarchyComponent>(entity);
    hierarchy.parent = parent;
  }

  // Mesh
  if (node.meshIndex >= 0) {
    const auto& mesh = model.meshes[node.meshIndex];

    ecs::MeshComponent meshComp;
    meshComp.vertexBuffer = mesh.vertexBuffer;
    meshComp.indexBuffer = mesh.indexBuffer;

    for (const auto& prim : mesh.primitives) {
      ecs::SubMesh subMesh;
      subMesh.indexOffset = prim.indexOffset;
      subMesh.indexCount = prim.indexCount;
      subMesh.vertexOffset = prim.vertexOffset;
      subMesh.materialIndex = prim.materialIndex >= 0 ? prim.materialIndex : 0;
      meshComp.subMeshes.push_back(subMesh);
    }

    registry.emplace<ecs::MeshComponent>(entity, std::move(meshComp));
    registry.emplace<ecs::BoundingBoxComponent>(entity, mesh.bounds);
    registry.emplace<ecs::RenderableComponent>(entity);

    // Add MaterialComponent with GPU materials
    ecs::MaterialComponent matComp;
    matComp.gpuMaterials = gpuMaterials;
    registry.emplace<ecs::MaterialComponent>(entity, std::move(matComp));
  }

  // Light
  if (node.lightIndex >= 0 &&
      node.lightIndex < static_cast<int32_t>(model.lights.size())) {
    const auto& light = model.lights[node.lightIndex];

    switch (light.type) {
      case Light::Type::Directional: {
        ecs::DirectionalLightComponent dirLight;
        dirLight.direction = glm::vec3(0.0F, 0.0F, -1.0F);
        dirLight.color = light.color;
        dirLight.intensity = light.intensity;
        registry.emplace<ecs::DirectionalLightComponent>(entity, dirLight);
        break;
      }
      case Light::Type::Point: {
        ecs::PointLightComponent pointLight;
        pointLight.color = light.color;
        pointLight.intensity = light.intensity;
        pointLight.radius = light.range;
        registry.emplace<ecs::PointLightComponent>(entity, pointLight);
        break;
      }
      case Light::Type::Spot: {
        ecs::SpotLightComponent spotLight;
        spotLight.direction = glm::vec3(0.0F, 0.0F, -1.0F);
        spotLight.color = light.color;
        spotLight.intensity = light.intensity;
        spotLight.innerConeAngle = light.innerConeAngle;
        spotLight.outerConeAngle = light.outerConeAngle;
        spotLight.radius = light.range;
        registry.emplace<ecs::SpotLightComponent>(entity, spotLight);
        break;
      }
    }
  }

  // Camera
  if (node.cameraIndex >= 0 &&
      node.cameraIndex < static_cast<int32_t>(model.cameras.size())) {
    ecs::CameraComponent camComp;
    registry.emplace<ecs::CameraComponent>(entity, camComp);
  }

  // Recurse children
  for (uint32_t childIndex : node.children) {
    InstantiateNode(registry, model, gpuMaterials, childIndex, entity);
  }

  return entity;
}

}  // namespace resource
