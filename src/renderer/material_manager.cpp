#include "renderer/material_manager.hpp"

#include <cstring>

#include "logger.hpp"

namespace renderer {

MaterialManager::MaterialManager(rhi::Factory& factory) : factory_{factory} {}

void MaterialManager::Initialize(rhi::DescriptorSetLayout* materialLayout) {
  materialLayout_ = materialLayout;
  CreateDefaultTextures();

  // Create default material
  resource::Material defaultMat;
  defaultMat.name = "Default";
  defaultMat.baseColorFactor = glm::vec4(1.0F);
  defaultMat.metallicFactor = 0.0F;
  defaultMat.roughnessFactor = 0.5F;
  defaultMat.alphaCutoff = 0.5F;

  defaultMaterial_ = std::make_unique<GPUMaterial>();

  // Create uniform buffer
  defaultMaterial_->uniformBuffer = factory_.CreateBuffer(
      sizeof(GPUMaterialUniforms), rhi::BufferUsage::Uniform,
      rhi::MemoryUsage::CPUToGPU);

  GPUMaterialUniforms uniforms{
      .baseColorFactor = defaultMat.baseColorFactor,
      .metallicFactor = defaultMat.metallicFactor,
      .roughnessFactor = defaultMat.roughnessFactor,
      .alphaCutoff = defaultMat.alphaCutoff,
  };

  void* data = defaultMaterial_->uniformBuffer->Map();
  std::memcpy(data, &uniforms, sizeof(GPUMaterialUniforms));
  defaultMaterial_->uniformBuffer->Unmap();

  // Create descriptor set
  defaultMaterial_->descriptorSet =
      factory_.CreateDescriptorSet(materialLayout_);
  defaultMaterial_->descriptorSet->BindBuffer(
      0, defaultMaterial_->uniformBuffer.get(), 0, sizeof(GPUMaterialUniforms));
  defaultMaterial_->descriptorSet->BindTexture(1, whiteTexture_.get(),
                                               defaultSampler_);
  defaultMaterial_->descriptorSet->BindTexture(2, normalTexture_.get(),
                                               defaultSampler_);
  defaultMaterial_->descriptorSet->BindTexture(3, whiteTexture_.get(),
                                               defaultSampler_);

  defaultMaterial_->baseColorTexture = whiteTexture_;
  defaultMaterial_->normalTexture = normalTexture_;
  defaultMaterial_->metallicRoughnessTexture = whiteTexture_;

  LOG_INFO("MaterialManager initialized with default material");
}

void MaterialManager::CreateDefaultTextures() {
  // Create 1x1 white texture (RGBA8)
  {
    std::array<uint8_t, 4> whitePixel = {255, 255, 255, 255};
    whiteTexture_ = factory_.CreateTexture(1, 1, rhi::Format::R8G8B8A8Unorm,
                                           rhi::TextureUsage::Sampled);
    whiteTexture_->Upload(std::as_bytes(std::span{whitePixel}));
    LOG_DEBUG("Created 1x1 white texture");
  }

  // Create 1x1 flat normal texture (encoded as 0.5, 0.5, 1.0 = tangent space
  // up)
  {
    std::array<uint8_t, 4> normalPixel = {128, 128, 255, 255};
    normalTexture_ = factory_.CreateTexture(1, 1, rhi::Format::R8G8B8A8Unorm,
                                            rhi::TextureUsage::Sampled);
    normalTexture_->Upload(std::as_bytes(std::span{normalPixel}));
    LOG_DEBUG("Created 1x1 normal texture");
  }

  // Create 1x1 black texture
  {
    std::array<uint8_t, 4> blackPixel = {0, 0, 0, 255};
    blackTexture_ = factory_.CreateTexture(1, 1, rhi::Format::R8G8B8A8Unorm,
                                           rhi::TextureUsage::Sampled);
    blackTexture_->Upload(std::as_bytes(std::span{blackPixel}));
    LOG_DEBUG("Created 1x1 black texture");
  }
}

GPUMaterial* MaterialManager::CreateMaterial(
    const resource::Material& material,
    const std::vector<resource::TextureResource>& textures) {
  auto gpuMat = std::make_unique<GPUMaterial>();

  // Create uniform buffer
  gpuMat->uniformBuffer = factory_.CreateBuffer(sizeof(GPUMaterialUniforms),
                                                rhi::BufferUsage::Uniform,
                                                rhi::MemoryUsage::CPUToGPU);

  GPUMaterialUniforms uniforms{
      .baseColorFactor = material.baseColorFactor,
      .metallicFactor = material.metallicFactor,
      .roughnessFactor = material.roughnessFactor,
      .alphaCutoff = material.alphaCutoff,
  };

  void* data = gpuMat->uniformBuffer->Map();
  std::memcpy(data, &uniforms, sizeof(GPUMaterialUniforms));
  gpuMat->uniformBuffer->Unmap();

  // Resolve textures (use defaults if not specified)
  if (material.baseColorTexture >= 0 &&
      material.baseColorTexture < static_cast<int32_t>(textures.size()) &&
      textures[material.baseColorTexture].texture) {
    gpuMat->baseColorTexture = textures[material.baseColorTexture].texture;
  } else {
    gpuMat->baseColorTexture = whiteTexture_;
  }

  if (material.normalTexture >= 0 &&
      material.normalTexture < static_cast<int32_t>(textures.size()) &&
      textures[material.normalTexture].texture) {
    gpuMat->normalTexture = textures[material.normalTexture].texture;
  } else {
    gpuMat->normalTexture = normalTexture_;
  }

  if (material.metallicRoughnessTexture >= 0 &&
      material.metallicRoughnessTexture <
          static_cast<int32_t>(textures.size()) &&
      textures[material.metallicRoughnessTexture].texture) {
    gpuMat->metallicRoughnessTexture =
        textures[material.metallicRoughnessTexture].texture;
  } else {
    gpuMat->metallicRoughnessTexture = whiteTexture_;
  }

  // Create descriptor set
  gpuMat->descriptorSet = factory_.CreateDescriptorSet(materialLayout_);
  gpuMat->descriptorSet->BindBuffer(0, gpuMat->uniformBuffer.get(), 0,
                                    sizeof(GPUMaterialUniforms));
  gpuMat->descriptorSet->BindTexture(1, gpuMat->baseColorTexture.get(),
                                     defaultSampler_);
  gpuMat->descriptorSet->BindTexture(2, gpuMat->normalTexture.get(),
                                     defaultSampler_);
  gpuMat->descriptorSet->BindTexture(3, gpuMat->metallicRoughnessTexture.get(),
                                     defaultSampler_);

  LOG_DEBUG("Created GPU material: {}", material.name);

  GPUMaterial* result = gpuMat.get();
  materials_.push_back(std::move(gpuMat));
  return result;
}

}  // namespace renderer
