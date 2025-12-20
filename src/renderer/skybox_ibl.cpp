#include "renderer/skybox_ibl.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <numbers>

#include <glm/glm.hpp>

#include "logger.hpp"
#include "renderer/cube_mesh.hpp"

// NOLINTBEGIN
// stb_image is already included in model_loader.cpp, so just declare the
// functions
extern "C" {
float* stbi_loadf(const char* filename, int* x, int* y, int* comp,
                  int req_comp);
void stbi_image_free(void* retval_from_stbi_load);
}
// NOLINTEND

#include "logger.hpp"
#include "renderer/cube_mesh.hpp"

namespace renderer {

SkyboxIBL::SkyboxIBL(rhi::Device& device, rhi::Factory& factory)
    : device_{device}, factory_{factory} {}

void SkyboxIBL::Initialize() {
  // Create samplers
  cubemapSampler_ = factory_.CreateSampler(
      rhi::Filter::Linear, rhi::Filter::Linear, rhi::AddressMode::ClampToEdge);

  brdfSampler_ = factory_.CreateSampler(
      rhi::Filter::Linear, rhi::Filter::Linear, rhi::AddressMode::ClampToEdge);

  // Create IBL descriptor set layout
  std::array<rhi::DescriptorBinding, 4> iblBindings{
      rhi::DescriptorBinding{.binding = 0,
                             .type = rhi::DescriptorType::CombinedImageSampler,
                             .count = 1},
      rhi::DescriptorBinding{.binding = 1,
                             .type = rhi::DescriptorType::CombinedImageSampler,
                             .count = 1},
      rhi::DescriptorBinding{.binding = 2,
                             .type = rhi::DescriptorType::CombinedImageSampler,
                             .count = 1},
      rhi::DescriptorBinding{.binding = 3,
                             .type = rhi::DescriptorType::CombinedImageSampler,
                             .count = 1},
  };

  iblDescriptorLayout_ = factory_.CreateDescriptorSetLayout(iblBindings);

  // Create cube mesh for skybox
  CreateCubeMesh();

  // Create default procedural skybox
  CreateProceduralSky();

  LOG_INFO("SkyboxIBL system initialized");
}

void SkyboxIBL::CreateCubeMesh() {
  auto [vertices, indices] = GenerateCubeMesh();
  cubeIndexCount_ = static_cast<uint32_t>(indices.size());

  // Create vertex buffer
  size_t vertexDataSize = vertices.size() * sizeof(ecs::Vertex);
  cubeVertexBuffer_ = factory_.CreateBuffer(
      vertexDataSize, rhi::BufferUsage::Vertex, rhi::MemoryUsage::CPUToGPU);
  cubeVertexBuffer_->Upload(
      std::span<const std::byte>(
          std::bit_cast<const std::byte*>(vertices.data()), vertexDataSize),
      0);

  // Create index buffer
  size_t indexDataSize = indices.size() * sizeof(uint32_t);
  cubeIndexBuffer_ = factory_.CreateBuffer(
      indexDataSize, rhi::BufferUsage::Index, rhi::MemoryUsage::CPUToGPU);
  cubeIndexBuffer_->Upload(
      std::span<const std::byte>(
          std::bit_cast<const std::byte*>(indices.data()), indexDataSize),
      0);

  LOG_INFO("Created skybox cube mesh ({} vertices, {} indices)",
           vertices.size(), indices.size());
}

void SkyboxIBL::CreateProceduralSky() {
  CreateDefaultSkybox();
  GenerateIrradianceMap();
  GeneratePrefilteredMap();
  GenerateBRDFLUT();
  CreateIBLDescriptorSet();
}

void SkyboxIBL::CreateDefaultSkybox() {
  // Create a 256x256 cubemap with a simple gradient sky
  constexpr uint32_t kSize = 256;

  skyboxCubemap_ = factory_.CreateCubemap(
      kSize, rhi::Format::R8G8B8A8Unorm,
      rhi::TextureUsage::Sampled | rhi::TextureUsage::ColorAttachment);

  // Generate simple gradient data for each face
  std::vector<uint8_t> faceData(static_cast<size_t>(kSize * kSize * 4));

  for (uint32_t face = 0; face < 6; ++face) {
    for (uint32_t y = 0; y < kSize; ++y) {
      for (uint32_t x = 0; x < kSize; ++x) {
        uint32_t idx = (y * kSize + x) * 4;

        // Simple blue-to-cyan gradient from bottom to top
        float t = static_cast<float>(y) / static_cast<float>(kSize);

        faceData[idx + 0] = static_cast<uint8_t>(50.0F * (1.0F - t));  // R
        faceData[idx + 1] =
            static_cast<uint8_t>((100.0F * (1.0F - t)) + (150.0F * t));  // G
        faceData[idx + 2] =
            static_cast<uint8_t>((150.0F * (1.0F - t)) + (255.0F * t));  // B
        faceData[idx + 3] = 255;                                         // A
      }
    }

    // Upload to cubemap face
    skyboxCubemap_->Upload(
        std::span<const std::byte>(
            std::bit_cast<const std::byte*>(faceData.data()), faceData.size()),
        0, face);
  }

  LOG_INFO("Created default gradient skybox cubemap ({}x{})", kSize, kSize);
}

void SkyboxIBL::GenerateIrradianceMap() {
  // Create a smaller cubemap for diffuse irradiance (32x32 is enough)
  constexpr uint32_t kSize = 32;

  irradianceMap_ = factory_.CreateCubemap(
      kSize, rhi::Format::R8G8B8A8Unorm,
      rhi::TextureUsage::Sampled | rhi::TextureUsage::ColorAttachment);

  // For a simple procedural sky, just use a uniform ambient color
  std::vector<uint8_t> faceData(static_cast<size_t>(kSize * kSize * 4));

  // Fill with a soft DARK ambient color (much darker to avoid overexposure)
  for (uint32_t i = 0; i < kSize * kSize; ++i) {
    faceData[(i * 4) + 0] = 20;   // R - much darker
    faceData[(i * 4) + 1] = 25;   // G
    faceData[(i * 4) + 2] = 30;   // B
    faceData[(i * 4) + 3] = 255;  // A
  }

  for (uint32_t face = 0; face < 6; ++face) {
    irradianceMap_->Upload(
        std::span<const std::byte>(
            std::bit_cast<const std::byte*>(faceData.data()), faceData.size()),
        0, face);
  }

  LOG_INFO("Generated irradiance map ({}x{})", kSize, kSize);
}

void SkyboxIBL::GeneratePrefilteredMap() {
  // Create prefiltered environment map with multiple mip levels
  constexpr uint32_t kSize = 128;
  constexpr uint32_t kMipLevels = 5;

  prefilteredMap_ = factory_.CreateCubemap(
      kSize, rhi::Format::R8G8B8A8Unorm,
      rhi::TextureUsage::Sampled | rhi::TextureUsage::ColorAttachment,
      kMipLevels);

  // For each mip level, create slightly darker/blurrier version
  for (uint32_t mip = 0; mip < kMipLevels; ++mip) {
    uint32_t mipSize = kSize >> mip;
    std::vector<uint8_t> faceData(static_cast<size_t>(mipSize * mipSize * 4));

    float roughness =
        static_cast<float>(mip) / static_cast<float>(kMipLevels - 1);

    for (uint32_t i = 0; i < mipSize * mipSize; ++i) {
      // Much darker values to avoid overexposure
      float factor = (1.0F - (roughness * 0.5F)) * 0.3F;  // Scale down overall

      faceData[(i * 4) + 0] = static_cast<uint8_t>(50.0F * factor);   // R
      faceData[(i * 4) + 1] = static_cast<uint8_t>(100.0F * factor);  // G
      faceData[(i * 4) + 2] = static_cast<uint8_t>(150.0F * factor);  // B
      faceData[(i * 4) + 3] = 255;                                    // A
    }

    for (uint32_t face = 0; face < 6; ++face) {
      prefilteredMap_->Upload(
          std::span<const std::byte>(
              std::bit_cast<const std::byte*>(faceData.data()),
              faceData.size()),
          mip, face);
    }
  }

  LOG_INFO("Generated prefiltered environment map ({}x{}, {} mips)", kSize,
           kSize, kMipLevels);
}

void SkyboxIBL::GenerateBRDFLUT() {
  // Generate BRDF integration lookup table
  constexpr uint32_t kLutSize = 512;
  brdfLUT_ = factory_.CreateTexture(
      kLutSize, kLutSize, rhi::Format::R16G16Sfloat,
      rhi::TextureUsage::Sampled | rhi::TextureUsage::ColorAttachment);

  std::vector<uint16_t> lutData(static_cast<size_t>(kLutSize * kLutSize * 2));

  // Proper half-float conversion
  auto floatToHalf = [](float value) -> uint16_t {
    // IEEE 754 half-precision conversion
    uint32_t f = *std::bit_cast<const uint32_t*>(&value);
    uint32_t sign = (f >> 16) & 0x8000;
    auto exponent = static_cast<int32_t>(((f >> 23) & 0xFF) - 127 + 15);
    uint32_t mantissa = (f >> 13) & 0x3FF;

    if (exponent <= 0) {
      return static_cast<uint16_t>(sign);
    }
    if (exponent >= 31) {
      return static_cast<uint16_t>(sign | 0x7C00);
    }
    return static_cast<uint16_t>(sign | (exponent << 10) | mantissa);
  };

  for (uint32_t y = 0; y < kLutSize; ++y) {
    for (uint32_t x = 0; x < kLutSize; ++x) {
      float ndotV = static_cast<float>(x) / static_cast<float>(kLutSize - 1);
      float roughness =
          static_cast<float>(y) / static_cast<float>(kLutSize - 1);

      // Better approximation for BRDF integration
      // These should be in 0-1 range
      float scale = ndotV * (1.0F - roughness * 0.5F);
      float bias = 0.05F * roughness;

      uint32_t idx = (y * kLutSize + x) * 2;
      lutData[idx + 0] = floatToHalf(scale);
      lutData[idx + 1] = floatToHalf(bias);
    }
  }

  brdfLUT_->Upload(std::span<const std::byte>(
                       std::bit_cast<const std::byte*>(lutData.data()),
                       lutData.size() * sizeof(uint16_t)),
                   0, 0);

  LOG_INFO("Generated BRDF LUT ({}x{})", kLutSize, kLutSize);
}

void SkyboxIBL::CreateIBLDescriptorSet() {
  if (!iblDescriptorLayout_) {
    return;
  }

  iblDescriptorSet_ = factory_.CreateDescriptorSet(iblDescriptorLayout_.get());

  // Bind cubemap textures
  iblDescriptorSet_->BindTexture(0, skyboxCubemap_.get(),
                                 cubemapSampler_.get());
  iblDescriptorSet_->BindTexture(1, irradianceMap_.get(),
                                 cubemapSampler_.get());
  iblDescriptorSet_->BindTexture(2, prefilteredMap_.get(),
                                 cubemapSampler_.get());
  iblDescriptorSet_->BindTexture(3, brdfLUT_.get(), brdfSampler_.get());

  LOG_INFO("Created IBL descriptor set with all maps bound");
}

bool SkyboxIBL::LoadHDREnvironment(const std::string& hdrPath) {
  int width = 0;
  int height = 0;
  int channels = 0;

  float* hdrData = stbi_loadf(hdrPath.c_str(), &width, &height, &channels, 4);
  if (hdrData == nullptr) {
    LOG_WARNING("Failed to load HDR: {}", hdrPath);
    return false;
  }

  LOG_INFO("Loaded HDR environment: {}x{} from {}", width, height, hdrPath);

  // TODO: Create equirect texture for processing
  constexpr uint32_t kCubeSize = 512;

  skyboxCubemap_ = factory_.CreateCubemap(
      kCubeSize, rhi::Format::R16G16B16A16Sfloat,
      rhi::TextureUsage::Sampled | rhi::TextureUsage::ColorAttachment);

  // Convert equirectangular to cubemap
  std::vector<uint16_t> faceData(
      static_cast<size_t>(kCubeSize * kCubeSize * 4));

  auto floatToHalf = [](float value) -> uint16_t {
    if (value <= 0.0F) {
      return 0;
    }
    if (value >= 65504.0F) {
      return 0x7BFF;
    }

    uint32_t f = *std::bit_cast<uint32_t*>(&value);
    uint32_t sign = (f >> 16) & 0x8000;
    int32_t exponent = static_cast<int32_t>((f >> 23) & 0xFF) - 127 + 15;
    uint32_t mantissa = (f >> 13) & 0x3FF;

    if (exponent <= 0) {
      return static_cast<uint16_t>(sign);
    }
    if (exponent >= 31) {
      return static_cast<uint16_t>(sign | 0x7C00);
    }
    return static_cast<uint16_t>(sign | (exponent << 10) | mantissa);
  };

  // Face directions for cubemap
  // +X, -X, +Y, -Y, +Z, -Z
  const std::array<std::array<glm::vec3, 3>, 6> faceDirs{
      // +X
      std::array<glm::vec3, 3>{glm::vec3{0, 0, -1}, glm::vec3{0, -1, 0},
                               glm::vec3{1, 0, 0}},
      // -X
      std::array<glm::vec3, 3>{glm::vec3{0, 0, 1}, glm::vec3{0, -1, 0},
                               glm::vec3{-1, 0, 0}},
      // +Y
      std::array<glm::vec3, 3>{glm::vec3{1, 0, 0}, glm::vec3{0, 0, 1},
                               glm::vec3{0, 1, 0}},
      // -Y
      std::array<glm::vec3, 3>{glm::vec3{1, 0, 0}, glm::vec3{0, 0, -1},
                               glm::vec3{0, -1, 0}},
      // +Z
      std::array<glm::vec3, 3>{glm::vec3{1, 0, 0}, glm::vec3{0, -1, 0},
                               glm::vec3{0, 0, 1}},
      // -Z
      std::array<glm::vec3, 3>{glm::vec3{-1, 0, 0}, glm::vec3{0, -1, 0},
                               glm::vec3{0, 0, -1}},
  };

  for (uint32_t face = 0; face < 6; ++face) {
    for (uint32_t y = 0; y < kCubeSize; ++y) {
      for (uint32_t x = 0; x < kCubeSize; ++x) {
        // Map pixel to direction
        float u = ((static_cast<float>(x) + 0.5F) /
                   static_cast<float>(kCubeSize) * 2.0F) -
                  1.0F;
        float v = ((static_cast<float>(y) + 0.5F) /
                   static_cast<float>(kCubeSize) * 2.0F) -
                  1.0F;

        glm::vec3 dir = glm::normalize(
            faceDirs[face][2] + faceDirs[face][0] * u + faceDirs[face][1] * v);

        // Convert direction to equirectangular UV
        float theta = std::atan2(dir.z, dir.x);
        float phi = std::asin(glm::clamp(dir.y, -1.0F, 1.0F));

        float eqU = (theta / (2.0F * std::numbers::pi_v<float>)) + 0.5F;
        float eqV = (phi / std::numbers::pi_v<float>)+0.5F;

        // Sample HDR
        int hdrX = static_cast<int>(eqU * static_cast<float>(width)) % width;
        int hdrY = static_cast<int>((1.0F - eqV) * static_cast<float>(height)) %
                   height;
        if (hdrX < 0) {
          hdrX += width;
        }
        if (hdrY < 0) {
          hdrY += height;
        }

        int hdrIdx = (hdrY * width + hdrX) * 4;

        uint32_t idx = (y * kCubeSize + x) * 4;
        faceData[idx + 0] = floatToHalf(hdrData[hdrIdx + 0]);
        faceData[idx + 1] = floatToHalf(hdrData[hdrIdx + 1]);
        faceData[idx + 2] = floatToHalf(hdrData[hdrIdx + 2]);
        faceData[idx + 3] = floatToHalf(1.0F);
      }
    }

    skyboxCubemap_->Upload(std::span<const std::byte>(
                               std::bit_cast<const std::byte*>(faceData.data()),
                               faceData.size() * sizeof(uint16_t)),
                           0, face);
  }

  stbi_image_free(hdrData);

  // Regenerate IBL maps from the new skybox
  GenerateIrradianceMap();
  GeneratePrefilteredMap();
  CreateIBLDescriptorSet();

  LOG_INFO("Created HDR cubemap ({}x{}) from {}", kCubeSize, kCubeSize,
           hdrPath);
  return true;
}
}  // namespace renderer
