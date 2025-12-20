#pragma once

#include <memory>
#include <string>

#include "rhi/buffer.hpp"
#include "rhi/descriptor.hpp"
#include "rhi/device.hpp"
#include "rhi/factory.hpp"
#include "rhi/sampler.hpp"
#include "rhi/texture.hpp"

namespace renderer {

class SkyboxIBL {
 public:
  SkyboxIBL(rhi::Device& device, rhi::Factory& factory);

  void Initialize();

  void CreateProceduralSky();

  [[nodiscard]] rhi::DescriptorSet* GetIBLDescriptorSet() const {
    return iblDescriptorSet_.get();
  }

  [[nodiscard]] rhi::DescriptorSetLayout* GetIBLDescriptorLayout() const {
    return iblDescriptorLayout_.get();
  }

  [[nodiscard]] bool LoadHDREnvironment(const std::string& hdrPath);

  [[nodiscard]] bool IsLoaded() const { return skyboxCubemap_ != nullptr; }

  // Skybox mesh buffers
  [[nodiscard]] rhi::Buffer* GetCubeVertexBuffer() const {
    return cubeVertexBuffer_.get();
  }

  [[nodiscard]] rhi::Buffer* GetCubeIndexBuffer() const {
    return cubeIndexBuffer_.get();
  }

  [[nodiscard]] uint32_t GetCubeIndexCount() const { return cubeIndexCount_; }

 private:
  void CreateDefaultSkybox();
  void GenerateIrradianceMap();
  void GeneratePrefilteredMap();
  void GenerateBRDFLUT();
  void CreateIBLDescriptorSet();
  void CreateCubeMesh();

  rhi::Device& device_;
  rhi::Factory& factory_;

  // Cubemaps
  std::shared_ptr<rhi::Texture> skyboxCubemap_;
  std::shared_ptr<rhi::Texture> irradianceMap_;
  std::shared_ptr<rhi::Texture> prefilteredMap_;
  std::shared_ptr<rhi::Texture> brdfLUT_;

  // Descriptor resources
  std::unique_ptr<rhi::DescriptorSetLayout> iblDescriptorLayout_;
  std::unique_ptr<rhi::DescriptorSet> iblDescriptorSet_;
  std::unique_ptr<rhi::Sampler> cubemapSampler_;
  std::unique_ptr<rhi::Sampler> brdfSampler_;

  // Cube mesh for skybox rendering
  std::shared_ptr<rhi::Buffer> cubeVertexBuffer_;
  std::shared_ptr<rhi::Buffer> cubeIndexBuffer_;
  uint32_t cubeIndexCount_{0};
};

}  // namespace renderer
