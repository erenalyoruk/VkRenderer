#pragma once

#include <memory>
#include <vector>

#include "resource/types.hpp"
#include "rhi/buffer.hpp"
#include "rhi/descriptor.hpp"
#include "rhi/factory.hpp"
#include "rhi/sampler.hpp"
#include "rhi/texture.hpp"

namespace renderer {

struct GPUMaterialUniforms {
  alignas(16) glm::vec4 baseColorFactor;
  alignas(4) float metallicFactor;
  alignas(4) float roughnessFactor;
  alignas(4) float alphaCutoff;
  alignas(4) float _padding;  // NOLINT
};

struct GPUMaterial {
  std::unique_ptr<rhi::Buffer> uniformBuffer;
  std::unique_ptr<rhi::DescriptorSet> descriptorSet;

  // Keep references to textures to prevent destruction
  std::shared_ptr<rhi::Texture> baseColorTexture;
  std::shared_ptr<rhi::Texture> normalTexture;
  std::shared_ptr<rhi::Texture> metallicRoughnessTexture;
};

class MaterialManager {
 public:
  MaterialManager(rhi::Factory& factory);

  void SetSampler(rhi::Sampler& sampler) { defaultSampler_ = &sampler; }
  void Initialize(rhi::DescriptorSetLayout* materialLayout);

  // Create default textures (white, normal, etc.)
  void CreateDefaultTextures();

  // Create GPU material from resource material
  GPUMaterial* CreateMaterial(
      const resource::Material& material,
      const std::vector<resource::TextureResource>& textures);

  // Get default material for objects without materials
  [[nodiscard]] GPUMaterial* GetDefaultMaterial() {
    return defaultMaterial_.get();
  }
  [[nodiscard]] const GPUMaterial* GetDefaultMaterial() const {
    return defaultMaterial_.get();
  }

  // Get textures
  [[nodiscard]] rhi::Texture* GetWhiteTexture() const {
    return whiteTexture_.get();
  }
  [[nodiscard]] rhi::Texture* GetNormalTexture() const {
    return normalTexture_.get();
  }
  [[nodiscard]] rhi::Texture* GetBlackTexture() const {
    return blackTexture_.get();
  }

 private:
  rhi::Factory& factory_;
  rhi::Sampler* defaultSampler_{nullptr};
  rhi::DescriptorSetLayout* materialLayout_{nullptr};

  // Default textures
  std::shared_ptr<rhi::Texture> whiteTexture_;  // 1x1 white (1,1,1,1)
  std::shared_ptr<rhi::Texture>
      normalTexture_;                           // 1x1 flat normal (0.5,0.5,1,1)
  std::shared_ptr<rhi::Texture> blackTexture_;  // 1x1 black (0,0,0,1)

  // Default material (white, non-metallic)
  std::unique_ptr<GPUMaterial> defaultMaterial_;

  // Created materials
  std::vector<std::unique_ptr<GPUMaterial>> materials_;
};

}  // namespace renderer
