#pragma once

#include <memory>

#include "resource/types.hpp"
#include "rhi/descriptor.hpp"
#include "rhi/factory.hpp"
#include "rhi/sampler.hpp"
#include "rhi/texture.hpp"

namespace renderer {

struct GPUMaterial {
  std::unique_ptr<rhi::Buffer> uniformBuffer;
  std::unique_ptr<rhi::DescriptorSet> descriptorSet;
};

class MaterialManager {
 public:
  MaterialManager(rhi::Factory& factory,
                  rhi::DescriptorSetLayout* materialLayout);

  // Create GPU material from loaded material data
  GPUMaterial* CreateMaterial(
      const resource::Material& material,
      const std::vector<resource::TextureResource>& textures);

  // Get or create a white 1x1 default texture
  [[nodiscard]] rhi::Texture* GetDefaultWhiteTexture();
  [[nodiscard]] rhi::Texture* GetDefaultNormalTexture();
  [[nodiscard]] rhi::Sampler* GetDefaultSampler() {
    return defaultSampler_.get();
  }

 private:
  rhi::Factory& factory_;
  rhi::DescriptorSetLayout* materialLayout_;

  std::vector<std::unique_ptr<GPUMaterial>> materials_;
  std::unique_ptr<rhi::Sampler> defaultSampler_;
  std::shared_ptr<rhi::Texture> defaultWhiteTexture_;
  std::shared_ptr<rhi::Texture> defaultNormalTexture_;
};

}  // namespace renderer
