#include "renderer/forward_plus.hpp"

#include <algorithm>

#include "logger.hpp"
#include "rhi/shader_utils.hpp"

namespace renderer {

ForwardPlus::ForwardPlus(rhi::Factory& factory, rhi::Device& device)
    : factory_{factory}, device_{device} {}

void ForwardPlus::Initialize() {
  CreateBuffers();
  CreatePipeline();
  LOG_INFO("Forward+ lighting initialized (tile size: {}x{}, max lights: {})",
           kTileSize, kTileSize, kMaxLights);
}

void ForwardPlus::Shutdown() {
  lightCullShader_.reset();
  cullDescriptorLayout_.reset();
  cullPipelineLayout_.reset();
  cullPipeline_.reset();
  cullDescriptorSet_.reset();
  lightDescriptorLayout_.reset();
  lightDescriptorSet_.reset();
  lightBuffer_.reset();
  lightCullUniformBuffer_.reset();
  lightIndexBuffer_.reset();
  lightGridBuffer_.reset();
}

void ForwardPlus::CreateBuffers() {
  // Light buffer (input)
  lightBuffer_ = factory_.CreateBuffer(
      sizeof(GPULight) * kMaxLights,
      rhi::BufferUsage::Storage | rhi::BufferUsage::TransferDst,
      rhi::MemoryUsage::CPUToGPU);

  // Light cull uniforms
  lightCullUniformBuffer_ = factory_.CreateBuffer(sizeof(LightCullUniforms),
                                                  rhi::BufferUsage::Uniform,
                                                  rhi::MemoryUsage::CPUToGPU);

  // Calculate initial tile count
  UpdateTileCount();

  // Light index buffer - stores indices of lights affecting each tile
  // Format: [tile0_light0, tile0_light1, ..., tile1_light0, ...]
  uint32_t maxTiles = 256 * 256;  // Max supported resolution / tile size
  lightIndexBuffer_ = factory_.CreateBuffer(
      sizeof(uint32_t) * maxTiles * kMaxLightsPerTile,
      rhi::BufferUsage::Storage, rhi::MemoryUsage::GPUOnly);

  // Light grid buffer - per-tile offset and count
  // Format: [offset0, count0, offset1, count1, ...]
  lightGridBuffer_ = factory_.CreateBuffer(sizeof(glm::uvec2) * maxTiles,
                                           rhi::BufferUsage::Storage,
                                           rhi::MemoryUsage::GPUOnly);
}

void ForwardPlus::CreatePipeline() {
  lightCullShader_ =
      rhi::CreateShaderFromFile(factory_, "assets/shaders/light_cull.comp.spv",
                                rhi::ShaderStage::Compute);

  // Culling descriptor layout
  // binding 0: LightCullUniforms (uniform)
  // binding 1: GPULight[] (storage, read)
  // binding 2: LightIndexBuffer (storage, write)
  // binding 3: LightGridBuffer (storage, write)
  std::array<rhi::DescriptorBinding, 4> cullBindings = {{
      {.binding = 0, .type = rhi::DescriptorType::UniformBuffer, .count = 1},
      {.binding = 1, .type = rhi::DescriptorType::StorageBuffer, .count = 1},
      {.binding = 2, .type = rhi::DescriptorType::StorageBuffer, .count = 1},
      {.binding = 3, .type = rhi::DescriptorType::StorageBuffer, .count = 1},
  }};
  cullDescriptorLayout_ = factory_.CreateDescriptorSetLayout(cullBindings);

  // Pipeline layout
  std::array<const rhi::DescriptorSetLayout*, 1> layouts = {
      cullDescriptorLayout_.get()};
  cullPipelineLayout_ = factory_.CreatePipelineLayout(layouts);

  // Compute pipeline
  rhi::ComputePipelineDesc desc{
      .computeShader = lightCullShader_.get(),
      .layout = cullPipelineLayout_.get(),
  };
  cullPipeline_ = factory_.CreateComputePipeline(desc);

  // Culling descriptor set
  cullDescriptorSet_ =
      factory_.CreateDescriptorSet(cullDescriptorLayout_.get());
  cullDescriptorSet_->BindBuffer(0, lightCullUniformBuffer_.get(), 0,
                                 sizeof(LightCullUniforms));
  cullDescriptorSet_->BindStorageBuffer(1, lightBuffer_.get(), 0,
                                        sizeof(GPULight) * kMaxLights);
  cullDescriptorSet_->BindStorageBuffer(
      2, lightIndexBuffer_.get(), 0,
      sizeof(uint32_t) * 256 * 256 * kMaxLightsPerTile);
  cullDescriptorSet_->BindStorageBuffer(3, lightGridBuffer_.get(), 0,
                                        sizeof(glm::uvec2) * 256 * 256);

  // Light descriptor layout for graphics pipeline (set 4)
  // binding 0: GPULight[] (storage, read)
  // binding 1: LightIndexBuffer (storage, read)
  // binding 2: LightGridBuffer (storage, read)
  // binding 3: LightCullUniforms (uniform) - for tile info
  std::array<rhi::DescriptorBinding, 4> lightBindings = {{
      {.binding = 0, .type = rhi::DescriptorType::StorageBuffer, .count = 1},
      {.binding = 1, .type = rhi::DescriptorType::StorageBuffer, .count = 1},
      {.binding = 2, .type = rhi::DescriptorType::StorageBuffer, .count = 1},
      {.binding = 3, .type = rhi::DescriptorType::UniformBuffer, .count = 1},
  }};
  lightDescriptorLayout_ = factory_.CreateDescriptorSetLayout(lightBindings);

  lightDescriptorSet_ =
      factory_.CreateDescriptorSet(lightDescriptorLayout_.get());
  lightDescriptorSet_->BindStorageBuffer(0, lightBuffer_.get(), 0,
                                         sizeof(GPULight) * kMaxLights);
  lightDescriptorSet_->BindStorageBuffer(
      1, lightIndexBuffer_.get(), 0,
      sizeof(uint32_t) * 256 * 256 * kMaxLightsPerTile);
  lightDescriptorSet_->BindStorageBuffer(2, lightGridBuffer_.get(), 0,
                                         sizeof(glm::uvec2) * 256 * 256);
  lightDescriptorSet_->BindBuffer(3, lightCullUniformBuffer_.get(), 0,
                                  sizeof(LightCullUniforms));

  LOG_DEBUG("Forward+ light culling pipeline created");
}

void ForwardPlus::UpdateTileCount() {
  tileCount_.x = (screenWidth_ + kTileSize - 1) / kTileSize;
  tileCount_.y = (screenHeight_ + kTileSize - 1) / kTileSize;
}

void ForwardPlus::UpdateLights(std::span<const GPULight> lights) {
  lightCount_ = static_cast<uint32_t>(
      std::min(lights.size(), static_cast<size_t>(kMaxLights)));

  if (lightCount_ > 0) {
    void* data = lightBuffer_->Map();
    std::memcpy(data, lights.data(), sizeof(GPULight) * lightCount_);
    lightBuffer_->Unmap();
  }
}

void ForwardPlus::UpdateScreenSize(uint32_t width, uint32_t height) {
  if (width == 0 || height == 0) {
    return;
  }

  screenWidth_ = width;
  screenHeight_ = height;
  UpdateTileCount();
}

void ForwardPlus::UpdateCamera(const glm::mat4& view,
                               const glm::mat4& projection, float nearPlane,
                               float farPlane) {
  cullUniforms_.view = view;
  cullUniforms_.projection = projection;
  cullUniforms_.invProjection = glm::inverse(projection);
  cullUniforms_.screenDimensions =
      glm::uvec4(screenWidth_, screenHeight_, tileCount_.x, tileCount_.y);
  cullUniforms_.lightCount = lightCount_;
  cullUniforms_.nearPlane = nearPlane;
  cullUniforms_.farPlane = farPlane;

  void* data = lightCullUniformBuffer_->Map();
  std::memcpy(data, &cullUniforms_, sizeof(LightCullUniforms));
  lightCullUniformBuffer_->Unmap();
}

void ForwardPlus::ExecuteLightCulling(rhi::CommandBuffer* cmd) {
  if (lightCount_ == 0 || cullPipeline_ == nullptr) {
    return;
  }

  cmd->BindPipeline(cullPipeline_.get());

  std::array<const rhi::DescriptorSet*, 1> sets = {cullDescriptorSet_.get()};
  cmd->BindDescriptorSets(cullPipeline_.get(), 0, sets);

  // Dispatch one workgroup per tile
  cmd->Dispatch(tileCount_.x, tileCount_.y, 1);

  // Barrier: compute writes -> fragment shader read
  cmd->BufferBarrier(lightIndexBuffer_.get(), rhi::AccessFlags::ShaderWrite,
                     rhi::AccessFlags::ShaderRead);
  cmd->BufferBarrier(lightGridBuffer_.get(), rhi::AccessFlags::ShaderWrite,
                     rhi::AccessFlags::ShaderRead);
}

}  // namespace renderer
