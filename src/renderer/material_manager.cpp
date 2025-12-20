#include "renderer/material_manager.hpp"

#include <array>
#include <bit>
#include <cstring>

namespace renderer {

struct MaterialUniformData {
  alignas(16) glm::vec4 baseColorFactor;
  alignas(4) float metallicFactor;
  alignas(4) float roughnessFactor;
  alignas(4) float alphaCutoff;
  alignas(4) float _padding;  // NOLINT
};

MaterialManager::MaterialManager(rhi::Factory& factory,
                                 rhi::DescriptorSetLayout* materialLayout)
    : factory_{factory}, materialLayout_{materialLayout} {
  // Create default sampler
  defaultSampler_ = factory_.CreateSampler(
      rhi::Filter::Linear, rhi::Filter::Linear, rhi::AddressMode::Repeat);

  // Create default white texture (1x1)
  defaultWhiteTexture_ = factory_.CreateTexture(
      1, 1, rhi::Format::R8G8B8A8Unorm, rhi::TextureUsage::Sampled);
  std::array<uint8_t, 4> whitePixel = {255, 255, 255, 255};
  std::span<const std::byte> whiteData{
      std::bit_cast<const std::byte*>(whitePixel.data()), 4};
  defaultWhiteTexture_->Upload(whiteData);

  // Create default normal texture (1x1, pointing up: 0.5, 0.5, 1.0)
  defaultNormalTexture_ = factory_.CreateTexture(
      1, 1, rhi::Format::R8G8B8A8Unorm, rhi::TextureUsage::Sampled);
  std::array<uint8_t, 4> normalPixel = {128, 128, 255, 255};
  std::span<const std::byte> normalData{
      std::bit_cast<const std::byte*>(normalPixel.data()), 4};
  defaultNormalTexture_->Upload(normalData);
}

GPUMaterial* MaterialManager::CreateMaterial(
    const resource::Material& material,
    const std::vector<resource::TextureResource>& textures) {
  auto gpuMat = std::make_unique<GPUMaterial>();

  // Create uniform buffer
  gpuMat->uniformBuffer = factory_.CreateBuffer(sizeof(MaterialUniformData),
                                                rhi::BufferUsage::Uniform,
                                                rhi::MemoryUsage::CPUToGPU);

  MaterialUniformData uniformData{
      .baseColorFactor = material.baseColorFactor,
      .metallicFactor = material.metallicFactor,
      .roughnessFactor = material.roughnessFactor,
      .alphaCutoff = material.alphaCutoff,
  };

  void* mapped = gpuMat->uniformBuffer->Map();
  std::memcpy(mapped, &uniformData, sizeof(uniformData));
  gpuMat->uniformBuffer->Unmap();

  // Create descriptor set
  gpuMat->descriptorSet = factory_.CreateDescriptorSet(materialLayout_);

  // Bind uniform buffer
  gpuMat->descriptorSet->BindBuffer(0, gpuMat->uniformBuffer.get(), 0,
                                    sizeof(MaterialUniformData));

  // Bind textures (use defaults if not present)
  rhi::Texture* baseColorTex = GetDefaultWhiteTexture();
  if (material.baseColorTexture >= 0 &&
      material.baseColorTexture < static_cast<int32_t>(textures.size())) {
    baseColorTex = textures[material.baseColorTexture].texture.get();
  }

  rhi::Texture* normalTex = GetDefaultNormalTexture();
  if (material.normalTexture >= 0 &&
      material.normalTexture < static_cast<int32_t>(textures.size())) {
    normalTex = textures[material.normalTexture].texture.get();
  }

  rhi::Texture* metallicRoughnessTex = GetDefaultWhiteTexture();
  if (material.metallicRoughnessTexture >= 0 &&
      material.metallicRoughnessTexture <
          static_cast<int32_t>(textures.size())) {
    metallicRoughnessTex =
        textures[material.metallicRoughnessTexture].texture.get();
  }

  gpuMat->descriptorSet->BindTexture(1, baseColorTex, defaultSampler_.get());
  gpuMat->descriptorSet->BindTexture(2, normalTex, defaultSampler_.get());
  gpuMat->descriptorSet->BindTexture(3, metallicRoughnessTex,
                                     defaultSampler_.get());

  materials_.push_back(std::move(gpuMat));
  return materials_.back().get();
}

rhi::Texture* MaterialManager::GetDefaultWhiteTexture() {
  return defaultWhiteTexture_.get();
}

rhi::Texture* MaterialManager::GetDefaultNormalTexture() {
  return defaultNormalTexture_.get();
}

}  // namespace renderer
