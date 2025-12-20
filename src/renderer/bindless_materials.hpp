#pragma once

#include <memory>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>

#include "resource/types.hpp"
#include "rhi/buffer.hpp"
#include "rhi/descriptor.hpp"
#include "rhi/factory.hpp"
#include "rhi/sampler.hpp"
#include "rhi/texture.hpp"

namespace renderer {

// GPU material data - must match shader struct
struct alignas(16) BindlessMaterialData {
  glm::vec4 baseColorFactor{1.0F};
  glm::vec4 emissiveFactorAndMetallic{0.0F, 0.0F, 0.0F,
                                      0.0F};  // xyz=emissive, w=metallic
  glm::vec4 roughnessAlphaCutoffOcclusion{
      1.0F, 0.5F, 1.0F, 0.0F};  // x=roughness, y=alpha, z=occlusion, w=pad
  uint32_t baseColorTexIdx{0};
  uint32_t normalTexIdx{0};
  uint32_t metallicRoughnessTexIdx{0};
  uint32_t occlusionTexIdx{0};
  uint32_t emissiveTexIdx{0};
  uint32_t _padding[3]{0, 0, 0};  // NOLINT
};

class BindlessMaterialManager {
 public:
  static constexpr uint32_t kMaxTextures = 1024;
  static constexpr uint32_t kMaxMaterials = 1024;

  BindlessMaterialManager(rhi::Factory& factory);

  void Initialize();

  // Register a texture and get its bindless index
  uint32_t RegisterTexture(const std::shared_ptr<rhi::Texture>& texture);

  // Register a material and get its index
  uint32_t RegisterMaterial(
      const resource::Material& material,
      const std::vector<resource::TextureResource>& textureResources);

  // Update material buffer on GPU
  void UpdateMaterialBuffer();

  // Get descriptor set for binding (set 1)
  [[nodiscard]] rhi::DescriptorSet* GetDescriptorSet() const {
    return descriptorSet_.get();
  }
  [[nodiscard]] rhi::DescriptorSetLayout* GetDescriptorLayout() const {
    return descriptorLayout_.get();
  }

  // Default texture indices
  [[nodiscard]] uint32_t GetWhiteTextureIndex() const {
    return whiteTextureIdx_;
  }
  [[nodiscard]] uint32_t GetNormalTextureIndex() const {
    return normalTextureIdx_;
  }
  [[nodiscard]] uint32_t GetBlackTextureIndex() const {
    return blackTextureIdx_;
  }

 private:
  void CreateDefaultTextures();

  rhi::Factory& factory_;
  std::unique_ptr<rhi::Sampler> sampler_;

  // Descriptor layout and set
  std::unique_ptr<rhi::DescriptorSetLayout> descriptorLayout_;
  std::unique_ptr<rhi::DescriptorSet> descriptorSet_;

  // Material SSBO
  std::unique_ptr<rhi::Buffer> materialBuffer_;
  std::vector<BindlessMaterialData> materials_;
  bool materialsDirty_{false};

  // Bindless texture array
  std::vector<std::shared_ptr<rhi::Texture>> textures_;
  std::unordered_map<rhi::Texture*, uint32_t> textureIndexMap_;

  // Default textures
  std::shared_ptr<rhi::Texture> whiteTexture_;
  std::shared_ptr<rhi::Texture> normalTexture_;
  std::shared_ptr<rhi::Texture> blackTexture_;
  uint32_t whiteTextureIdx_{0};
  uint32_t normalTextureIdx_{0};
  uint32_t blackTextureIdx_{0};
};

}  // namespace renderer
