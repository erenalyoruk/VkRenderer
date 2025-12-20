#pragma once

#include <memory>

#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "rhi/buffer.hpp"
#include "rhi/pipeline.hpp"
#include "rhi/texture.hpp"

namespace renderer {
class GPUMaterial;
}

namespace ecs {
// ============================================================================
// Transform Components
// ============================================================================

struct TransformComponent {
  glm::vec3 position{0.0F};
  glm::quat rotation{1.0F, 0.0F, 0.0F, 0.0F};  // Identity quaternion
  glm::vec3 scale{1.0F};

  [[nodiscard]] glm::mat4 GetMatrix() const {
    glm::mat4 transform{1.0F};
    transform = glm::translate(transform, position);
    transform *= glm::mat4_cast(rotation);
    transform = glm::scale(transform, scale);
    return transform;
  }
};

struct WorldTransformComponent {
  glm::mat4 matrix{1.0F};
};

// ============================================================================
// Hierarchy Components
// ============================================================================

struct HierarchyComponent {
  entt::entity parent{entt::null};
  std::vector<entt::entity> children;
};

// ============================================================================
// Mesh Components
// ============================================================================

struct Vertex {
  glm::vec3 position{0.0F};
  glm::vec3 normal{0.0F};
  glm::vec4 tangent{0.0F, 0.0F, 0.0F, 1.0F};
  glm::vec2 texCoord{0.0F};
  glm::vec4 color{1.0F};

  static std::vector<rhi::VertexBinding> GetBindings() {
    return {{
        .binding = 0,
        .stride = sizeof(Vertex),
        .inputRate = rhi::VertexInputRate::Vertex,
    }};
  }

  static std::vector<rhi::VertexAttribute> GetAttributes() {
    return {
        {.location = 0,
         .binding = 0,
         .format = rhi::Format::R32G32B32Sfloat,
         .offset = offsetof(Vertex, position)},
        {.location = 1,
         .binding = 0,
         .format = rhi::Format::R32G32B32Sfloat,
         .offset = offsetof(Vertex, normal)},
        {.location = 2,
         .binding = 0,
         .format = rhi::Format::R32G32B32A32Sfloat,
         .offset = offsetof(Vertex, tangent)},
        {.location = 3,
         .binding = 0,
         .format = rhi::Format::R32G32Sfloat,
         .offset = offsetof(Vertex, texCoord)},
        {.location = 4,
         .binding = 0,
         .format = rhi::Format::R32G32B32A32Sfloat,
         .offset = offsetof(Vertex, color)},
    };
  }
};

struct SubMesh {
  uint32_t indexOffset{0};
  uint32_t indexCount{0};
  uint32_t vertexOffset{0};
  uint32_t materialIndex{0};
};

struct MeshComponent {
  std::shared_ptr<rhi::Buffer> vertexBuffer;
  std::shared_ptr<rhi::Buffer> indexBuffer;
  std::vector<SubMesh> subMeshes;
  uint32_t vertexCount{0};
  uint32_t indexCount{0};
};

// ============================================================================
// Material Components
// ============================================================================

struct PBRMaterial {
  glm::vec4 baseColor{1.0F};
  float metallic{0.0F};
  float roughness{0.5F};
  float _padding[2];  // NOLINT

  std::shared_ptr<rhi::Texture> albedoTexture;
  std::shared_ptr<rhi::Texture> normalTexture;
  std::shared_ptr<rhi::Texture> metallicRoughnessTexture;
};

struct MaterialComponent {
  // Bindless material indices (one per submesh)
  std::vector<uint32_t> materialIndices;
};

// ============================================================================
// Rendering Components
// ============================================================================

struct RenderableComponent {
  bool castsShadows{true};
  bool receiveShadows{true};
  uint32_t renderLayer{0};  // For sorting/filtering
};

struct BoundingBoxComponent {
  glm::vec3 min{-1.0F};
  glm::vec3 max{1.0F};

  [[nodiscard]] glm::vec3 GetCenter() const { return (min + max) * 0.5F; }
  [[nodiscard]] glm::vec3 GetExtents() const { return (max - min) * 0.5F; }
};

// ============================================================================
// Lighting Components
// ============================================================================

struct DirectionalLightComponent {
  glm::vec3 direction{0.0F, -1.0F, 0.0F};
  glm::vec3 color{1.0F};
  float intensity{1.0F};
};

struct PointLightComponent {
  glm::vec3 color{1.0F};
  float intensity{1.0F};
  float radius{10.0F};
};

struct SpotLightComponent {
  glm::vec3 direction{0.0F, -1.0F, 0.0F};
  glm::vec3 color{1.0F};
  float intensity{1.0F};
  float innerConeAngle{0.5F};
  float outerConeAngle{0.75F};
  float radius{10.0F};
};

// ============================================================================
// Camera Component
// ============================================================================

struct CameraComponent {
  glm::mat4 view{1.0F};
  glm::mat4 projection{1.0F};
  std::array<glm::vec4, 6> frustumPlanes{};
  bool isActive{true};
};

// ============================================================================
// Tag Components
// ============================================================================

struct MainCameraTag {};
struct StaticTag {};
struct DynamicTag {};
}  // namespace ecs
