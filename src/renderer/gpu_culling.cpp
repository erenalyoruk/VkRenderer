#include "renderer/gpu_culling.hpp"

#include <cstring>
#include <fstream>

#include "logger.hpp"

namespace renderer {

GPUCulling::GPUCulling(rhi::Factory& factory, rhi::Device& device)
    : factory_{factory}, device_{device} {}

void GPUCulling::Initialize() {
  CreateBuffers();
  CreatePipeline();
  LOG_INFO("GPU Culling system initialized (max {} objects)", maxObjects_);
}

void GPUCulling::CreateBuffers() {
  // Object data buffer (input) - transforms, bounds, material indices
  objectBuffer_ = factory_.CreateBuffer(
      sizeof(ObjectData) * maxObjects_,
      rhi::BufferUsage::Storage | rhi::BufferUsage::TransferDst,
      rhi::MemoryUsage::CPUToGPU);

  // Cull uniforms buffer
  cullUniformBuffer_ =
      factory_.CreateBuffer(sizeof(CullUniforms), rhi::BufferUsage::Uniform,
                            rhi::MemoryUsage::CPUToGPU);

  // Draw command buffer (output)
  drawCommandBuffer_ = factory_.CreateBuffer(
      sizeof(DrawIndexedIndirectCommand) * maxObjects_,
      rhi::BufferUsage::Storage | rhi::BufferUsage::Indirect,
      rhi::MemoryUsage::GPUOnly);

  // Draw count buffer (output - atomic counter)
  // Extra 4 bytes at start for reset via transfer
  drawCountBuffer_ = factory_.CreateBuffer(
      sizeof(uint32_t) * 2,  // [0] = count, [1] = zero for reset
      rhi::BufferUsage::Storage | rhi::BufferUsage::Indirect |
          rhi::BufferUsage::TransferDst,
      rhi::MemoryUsage::GPUOnly);
}

void GPUCulling::CreatePipeline() {
  // Load compute shader
  std::ifstream file("assets/shaders/cull.comp.spv", std::ios::binary);
  if (!file) {
    LOG_ERROR("Failed to load cull.comp.spv");
    return;
  }

  std::vector<char> buffer((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
  std::vector<uint32_t> spirv(buffer.size() / 4);
  std::memcpy(spirv.data(), buffer.data(), buffer.size());

  cullShader_ = factory_.CreateShader(rhi::ShaderStage::Compute, spirv);

  // Culling descriptor layout
  // binding 0: CullUniforms (uniform)
  // binding 1: ObjectData[] (storage, read)
  // binding 2: DrawCommands[] (storage, write)
  // binding 3: DrawCount (storage, write)
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
      .computeShader = cullShader_.get(),
      .layout = cullPipelineLayout_.get(),
  };
  cullPipeline_ = factory_.CreateComputePipeline(desc);

  // Culling descriptor set
  cullDescriptorSet_ =
      factory_.CreateDescriptorSet(cullDescriptorLayout_.get());
  cullDescriptorSet_->BindBuffer(0, cullUniformBuffer_.get(), 0,
                                 sizeof(CullUniforms));
  cullDescriptorSet_->BindStorageBuffer(1, objectBuffer_.get(), 0,
                                        sizeof(ObjectData) * maxObjects_);
  cullDescriptorSet_->BindStorageBuffer(
      2, drawCommandBuffer_.get(), 0,
      sizeof(DrawIndexedIndirectCommand) * maxObjects_);
  cullDescriptorSet_->BindStorageBuffer(3, drawCountBuffer_.get(), 0,
                                        sizeof(uint32_t));

  // Object data descriptor layout for graphics pipeline (set 2)
  // binding 0: ObjectData[] (storage, read) - for fetching transforms in vertex
  // shader
  std::array<rhi::DescriptorBinding, 1> objectBindings = {{
      {.binding = 0, .type = rhi::DescriptorType::StorageBuffer, .count = 1},
  }};
  objectDescriptorLayout_ = factory_.CreateDescriptorSetLayout(objectBindings);

  objectDescriptorSet_ =
      factory_.CreateDescriptorSet(objectDescriptorLayout_.get());
  objectDescriptorSet_->BindStorageBuffer(0, objectBuffer_.get(), 0,
                                          sizeof(ObjectData) * maxObjects_);

  LOG_DEBUG("GPU Culling pipeline created");
}

void GPUCulling::UpdateObjects(std::span<const ObjectData> objects) {
  objectCount_ = static_cast<uint32_t>(objects.size());
  if (objectCount_ > maxObjects_) {
    LOG_WARNING("Object count {} exceeds max {}", objectCount_, maxObjects_);
    objectCount_ = maxObjects_;
  }

  if (objectCount_ > 0) {
    void* data = objectBuffer_->Map();
    std::memcpy(data, objects.data(), sizeof(ObjectData) * objectCount_);
    objectBuffer_->Unmap();
  }
}

void GPUCulling::ExtractFrustumPlanes(const glm::mat4& viewProj,
                                      glm::vec4* planes) {
  (void)this;

  // Left
  planes[0] = glm::vec4(
      viewProj[0][3] + viewProj[0][0], viewProj[1][3] + viewProj[1][0],
      viewProj[2][3] + viewProj[2][0], viewProj[3][3] + viewProj[3][0]);
  // Right
  planes[1] = glm::vec4(
      viewProj[0][3] - viewProj[0][0], viewProj[1][3] - viewProj[1][0],
      viewProj[2][3] - viewProj[2][0], viewProj[3][3] - viewProj[3][0]);
  // Bottom
  planes[2] = glm::vec4(
      viewProj[0][3] + viewProj[0][1], viewProj[1][3] + viewProj[1][1],
      viewProj[2][3] + viewProj[2][1], viewProj[3][3] + viewProj[3][1]);
  // Top
  planes[3] = glm::vec4(
      viewProj[0][3] - viewProj[0][1], viewProj[1][3] - viewProj[1][1],
      viewProj[2][3] - viewProj[2][1], viewProj[3][3] - viewProj[3][1]);
  // Near
  planes[4] = glm::vec4(
      viewProj[0][3] + viewProj[0][2], viewProj[1][3] + viewProj[1][2],
      viewProj[2][3] + viewProj[2][2], viewProj[3][3] + viewProj[3][2]);
  // Far
  planes[5] = glm::vec4(
      viewProj[0][3] - viewProj[0][2], viewProj[1][3] - viewProj[1][2],
      viewProj[2][3] - viewProj[2][2], viewProj[3][3] - viewProj[3][2]);

  // Normalize planes
  for (int i = 0; i < 6; ++i) {
    float len = glm::length(glm::vec3(planes[i]));
    if (len > 0.0001F) {
      planes[i] /= len;
    }
  }
}

void GPUCulling::UpdateFrustum(const glm::mat4& viewProjection) {
  CullUniforms uniforms{};
  uniforms.viewProjection = viewProjection;
  uniforms.objectCount = objectCount_;
  ExtractFrustumPlanes(viewProjection, uniforms.frustumPlanes.data());

  void* data = cullUniformBuffer_->Map();
  std::memcpy(data, &uniforms, sizeof(CullUniforms));
  cullUniformBuffer_->Unmap();
}

void GPUCulling::ResetDrawCount(rhi::CommandBuffer* cmd) {
  // Fill draw count buffer with zero
  cmd->FillBuffer(drawCountBuffer_.get(), 0, sizeof(uint32_t), 0);

  // Barrier to ensure fill completes before compute
  cmd->BufferBarrier(
      drawCountBuffer_.get(), rhi::AccessFlags::TransferWrite,
      rhi::AccessFlags::ShaderRead | rhi::AccessFlags::ShaderWrite);
}

void GPUCulling::Execute(rhi::CommandBuffer* cmd) {
  if (objectCount_ == 0 || cullPipeline_ == nullptr) {
    return;
  }

  cmd->BindPipeline(cullPipeline_.get());

  std::array<const rhi::DescriptorSet*, 1> sets = {cullDescriptorSet_.get()};
  cmd->BindDescriptorSets(cullPipeline_.get(), 0, sets);

  // Dispatch one thread per object
  uint32_t groupCount = (objectCount_ + 63) / 64;
  cmd->Dispatch(groupCount, 1, 1);

  // Barrier: compute writes -> indirect read + vertex shader read
  cmd->BufferBarrier(drawCommandBuffer_.get(), rhi::AccessFlags::ShaderWrite,
                     rhi::AccessFlags::IndirectCommandRead);
  cmd->BufferBarrier(drawCountBuffer_.get(), rhi::AccessFlags::ShaderWrite,
                     rhi::AccessFlags::IndirectCommandRead);
  cmd->BufferBarrier(objectBuffer_.get(), rhi::AccessFlags::ShaderWrite,
                     rhi::AccessFlags::ShaderRead);
}

}  // namespace renderer
