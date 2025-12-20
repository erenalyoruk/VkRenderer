#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "ecs/components.hpp"
#include "rhi/buffer.hpp"
#include "rhi/texture.hpp"

namespace resource {

// ============================================================================
// Mesh Resource
// ============================================================================
struct MeshPrimitive {
  uint32_t vertexOffset{0};
  uint32_t vertexCount{0};
  uint32_t indexOffset{0};
  uint32_t indexCount{0};
  int32_t materialIndex{-1};  // -1 = default material
};

struct Mesh {
  std::string name;
  std::shared_ptr<rhi::Buffer> vertexBuffer;
  std::shared_ptr<rhi::Buffer> indexBuffer;
  std::vector<MeshPrimitive> primitives;
  ecs::BoundingBoxComponent bounds;
};

// ============================================================================
// Material Resource
// ============================================================================
struct Material {
  std::string name;

  // PBR parameters
  glm::vec4 baseColorFactor{1.0F};
  float metallicFactor{1.0F};
  float roughnessFactor{1.0F};
  glm::vec3 emissiveFactor{0.0F};
  float alphaCutoff{0.5F};

  // Textures (indices into texture array, -1 = none)
  int32_t baseColorTexture{-1};
  int32_t metallicRoughnessTexture{-1};
  int32_t normalTexture{-1};
  int32_t occlusionTexture{-1};
  int32_t emissiveTexture{-1};

  // Alpha mode
  enum class AlphaMode : uint8_t { Opaque, Mask, Blend };
  AlphaMode alphaMode{AlphaMode::Opaque};

  bool doubleSided{false};
};

// ============================================================================
// Texture Resource
// ============================================================================
struct TextureResource {
  std::string name;
  std::shared_ptr<rhi::Texture> texture;
  uint32_t width{0};
  uint32_t height{0};
};

// ============================================================================
// Scene Node
// ============================================================================
struct SceneNode {
  std::string name;
  glm::vec3 translation{0.0F};
  glm::quat rotation{1.0F, 0.0F, 0.0F, 0.0F};
  glm::vec3 scale{1.0F};

  int32_t meshIndex{-1};  // -1 = no mesh
  int32_t cameraIndex{-1};
  int32_t lightIndex{-1};

  std::vector<uint32_t> children;
};

// ============================================================================
// Light
// ============================================================================
struct Light {
  enum class Type : uint8_t { Directional, Point, Spot };

  std::string name;
  Type type{Type::Point};
  glm::vec3 color{1.0F};
  float intensity{1.0F};
  float range{0.0F};  // 0 = infinite (for directional)
  float innerConeAngle{0.0F};
  float outerConeAngle{0.7854F};  // 45 degrees
};

// ============================================================================
// Camera
// ============================================================================
struct CameraData {
  std::string name;
  bool perspective{true};
  float yfov{0.7854F};          // 45 degrees
  float aspectRatio{1.77778F};  // 16:9
  float znear{0.1F};
  float zfar{1000.0F};
};

// ============================================================================
// Model (Complete loaded asset)
// ============================================================================
struct Model {
  std::string name;
  std::string sourcePath;

  std::vector<Mesh> meshes;
  std::vector<Material> materials;
  std::vector<TextureResource> textures;
  std::vector<SceneNode> nodes;
  std::vector<Light> lights;
  std::vector<CameraData> cameras;

  std::vector<uint32_t> rootNodes;  // Scene root node indices
};

// ============================================================================
// Handle types for resource references
// ============================================================================
using ModelHandle = size_t;
using TextureHandle = size_t;
using MeshHandle = size_t;

constexpr size_t kInvalidHandle = ~0ULL;

}  // namespace resource
