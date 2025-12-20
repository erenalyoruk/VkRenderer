#include "renderer/bindless_materials.hpp"

#include <cstring>

#include "logger.hpp"

namespace renderer {

BindlessMaterialManager::BindlessMaterialManager(rhi::Factory& factory)
    : factory_{factory} {}

void BindlessMaterialManager::Initialize() {
  // Create sampler
  sampler_ = factory_.CreateSampler(rhi::Filter::Linear, rhi::Filter::Linear,
                                    rhi::AddressMode::Repeat);

  // Descriptor layout for bindless materials (set 1)
  // binding 0: MaterialData[] SSBO
  // binding 1: sampler2D textures[] (bindless array)
  std::array<rhi::DescriptorBinding, 2> bindings = {{
      {.binding = 0, .type = rhi::DescriptorType::StorageBuffer, .count = 1},
      {.binding = 1,
       .type = rhi::DescriptorType::CombinedImageSampler,
       .count = kMaxTextures},
  }};
  descriptorLayout_ = factory_.CreateDescriptorSetLayout(bindings);

  // Material buffer
  materialBuffer_ = factory_.CreateBuffer(
      sizeof(BindlessMaterialData) * kMaxMaterials,
      rhi::BufferUsage::Storage | rhi::BufferUsage::TransferDst,
      rhi::MemoryUsage::CPUToGPU);

  // Create descriptor set BEFORE creating default textures
  descriptorSet_ = factory_.CreateDescriptorSet(descriptorLayout_.get());

  // Bind material buffer
  descriptorSet_->BindStorageBuffer(
      0, materialBuffer_.get(), 0,
      sizeof(BindlessMaterialData) * kMaxMaterials);

  // Now create default textures (this will call RegisterTexture which uses
  // descriptorSet_)
  CreateDefaultTextures();

  // Initialize remaining texture slots with white texture
  for (auto i = static_cast<uint32_t>(textures_.size()); i < kMaxTextures;
       ++i) {
    descriptorSet_->BindTexture(1, whiteTexture_.get(), sampler_.get(), i);
  }

  // Create default material (index 0)
  BindlessMaterialData defaultMat{};
  defaultMat.baseColorFactor = glm::vec4(1.0F);
  defaultMat.emissiveFactorAndMetallic = glm::vec4(0.0F, 0.0F, 0.0F, 0.0F);
  defaultMat.roughnessAlphaCutoffOcclusion = glm::vec4(1.0F, 0.5F, 1.0F, 0.0F);
  defaultMat.baseColorTexIdx = whiteTextureIdx_;
  defaultMat.normalTexIdx = normalTextureIdx_;
  defaultMat.metallicRoughnessTexIdx = whiteTextureIdx_;
  defaultMat.occlusionTexIdx = whiteTextureIdx_;
  defaultMat.emissiveTexIdx = blackTextureIdx_;
  materials_.push_back(defaultMat);
  materialsDirty_ = true;

  UpdateMaterialBuffer();

  LOG_INFO(
      "Bindless material manager initialized (max {} textures, {} materials)",
      kMaxTextures, kMaxMaterials);
}

void BindlessMaterialManager::CreateDefaultTextures() {
  // White texture (1x1)
  std::array<uint8_t, 4> whitePixel = {255, 255, 255, 255};
  whiteTexture_ = factory_.CreateTexture(1, 1, rhi::Format::R8G8B8A8Unorm,
                                         rhi::TextureUsage::Sampled);
  whiteTexture_->Upload(std::span<const std::byte>(
      std::bit_cast<const std::byte*>(whitePixel.data()), whitePixel.size()));
  whiteTextureIdx_ = RegisterTexture(whiteTexture_);

  // Normal texture (1x1 flat normal pointing up: 0.5, 0.5, 1.0)
  std::array<uint8_t, 4> normalPixel = {128, 128, 255, 255};
  normalTexture_ = factory_.CreateTexture(1, 1, rhi::Format::R8G8B8A8Unorm,
                                          rhi::TextureUsage::Sampled);
  normalTexture_->Upload(std::span<const std::byte>(
      std::bit_cast<const std::byte*>(normalPixel.data()), normalPixel.size()));
  normalTextureIdx_ = RegisterTexture(normalTexture_);

  // Black texture (1x1)
  std::array<uint8_t, 4> blackPixel = {0, 0, 0, 255};
  blackTexture_ = factory_.CreateTexture(1, 1, rhi::Format::R8G8B8A8Unorm,
                                         rhi::TextureUsage::Sampled);
  blackTexture_->Upload(std::span<const std::byte>(
      std::bit_cast<const std::byte*>(blackPixel.data()), blackPixel.size()));
  blackTextureIdx_ = RegisterTexture(blackTexture_);
}

uint32_t BindlessMaterialManager::RegisterTexture(
    const std::shared_ptr<rhi::Texture>& texture) {
  // Check if already registered
  auto it = textureIndexMap_.find(texture.get());
  if (it != textureIndexMap_.end()) {
    return it->second;
  }

  auto index = static_cast<uint32_t>(textures_.size());
  if (index >= kMaxTextures) {
    LOG_WARNING("Max texture count reached, returning white texture");
    return whiteTextureIdx_;
  }

  textures_.push_back(texture);
  textureIndexMap_[texture.get()] = index;

  // Update descriptor
  descriptorSet_->BindTexture(1, texture.get(), sampler_.get(), index);

  return index;
}

uint32_t BindlessMaterialManager::RegisterMaterial(
    const resource::Material& material,
    const std::vector<resource::TextureResource>& textureResources) {
  if (materials_.size() >= kMaxMaterials) {
    LOG_WARNING("Max material count reached, returning default material");
    return 0;
  }

  BindlessMaterialData matData{};
  matData.baseColorFactor = material.baseColorFactor;
  matData.emissiveFactorAndMetallic =
      glm::vec4(material.emissiveFactor.x, material.emissiveFactor.y,
                material.emissiveFactor.z, material.metallicFactor);
  matData.roughnessAlphaCutoffOcclusion =
      glm::vec4(material.roughnessFactor, material.alphaCutoff, 1.0F,
                0.0F);  // occlusionStrength defaults to 1.0

  // Register textures and get indices
  auto getTextureIndex = [&](int32_t texIdx, uint32_t defaultIdx) -> uint32_t {
    if (texIdx >= 0 && texIdx < static_cast<int32_t>(textureResources.size())) {
      const auto& texRes = textureResources[texIdx];
      if (texRes.texture) {
        return RegisterTexture(texRes.texture);
      }
    }
    return defaultIdx;
  };

  matData.baseColorTexIdx =
      getTextureIndex(material.baseColorTexture, whiteTextureIdx_);
  matData.normalTexIdx =
      getTextureIndex(material.normalTexture, normalTextureIdx_);
  matData.metallicRoughnessTexIdx =
      getTextureIndex(material.metallicRoughnessTexture, whiteTextureIdx_);
  matData.occlusionTexIdx =
      getTextureIndex(material.occlusionTexture, whiteTextureIdx_);
  matData.emissiveTexIdx =
      getTextureIndex(material.emissiveTexture, blackTextureIdx_);

  auto index = static_cast<uint32_t>(materials_.size());
  materials_.push_back(matData);
  materialsDirty_ = true;

  return index;
}

void BindlessMaterialManager::UpdateMaterialBuffer() {
  if (!materialsDirty_ || materials_.empty()) {
    return;
  }

  void* data = materialBuffer_->Map();
  std::memcpy(data, materials_.data(),
              sizeof(BindlessMaterialData) * materials_.size());
  materialBuffer_->Unmap();

  materialsDirty_ = false;
}

}  // namespace renderer
