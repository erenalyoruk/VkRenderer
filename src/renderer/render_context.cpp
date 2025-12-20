#include "render_context.hpp"

#include <array>

namespace renderer {

RenderContext::RenderContext(rhi::Device& device, rhi::Factory& factory)
    : device_{device}, factory_{factory} {
  CreateDescriptors();
  CreatePipeline();
  CreateFrameResources();
}

void RenderContext::CreateFrameResources() {
  for (auto& frame : frames_) {
    frame.inFlightFence = factory_.CreateFence(true);
    frame.imageAvailable = factory_.CreateSemaphore();
    frame.renderFinished = factory_.CreateSemaphore();

    frame.commandPool = factory_.CreateCommandPool(rhi::QueueType::Graphics);
    frame.commandBuffer = frame.commandPool->AllocateCommandBuffer();

    // Global uniform buffer
    frame.globalUniformBuffer =
        factory_.CreateBuffer(sizeof(GlobalUniforms), rhi::BufferUsage::Uniform,
                              rhi::MemoryUsage::CPUToGPU);

    // Object uniform buffer (allocate space for many objects)
    frame.objectUniformBuffer = factory_.CreateBuffer(
        sizeof(ObjectUniforms) * 10000, rhi::BufferUsage::Uniform,
        rhi::MemoryUsage::CPUToGPU);

    // Global descriptor set
    frame.globalDescriptorSet =
        factory_.CreateDescriptorSet(globalDescriptorLayout_.get());

    // Bind global uniform buffer to descriptor set
    frame.globalDescriptorSet->BindBuffer(0, frame.globalUniformBuffer.get(), 0,
                                          sizeof(GlobalUniforms));
  }
}

void RenderContext::CreateDescriptors() {
  // Global descriptor layout (set 0)
  std::array<rhi::DescriptorBinding, 1> globalBindings = {{
      {.binding = 0, .type = rhi::DescriptorType::UniformBuffer, .count = 1},
  }};
  globalDescriptorLayout_ = factory_.CreateDescriptorSetLayout(globalBindings);

  // Material descriptor layout (set 1)
  std::array<rhi::DescriptorBinding, 4> materialBindings = {{
      {.binding = 0, .type = rhi::DescriptorType::UniformBuffer, .count = 1},
      {.binding = 1, .type = rhi::DescriptorType::SampledImage, .count = 1},
      {.binding = 2, .type = rhi::DescriptorType::SampledImage, .count = 1},
      {.binding = 3, .type = rhi::DescriptorType::SampledImage, .count = 1},
  }};
  materialDescriptorLayout_ =
      factory_.CreateDescriptorSetLayout(materialBindings);

  // Default sampler
  defaultSampler_ = factory_.CreateSampler(
      rhi::Filter::Linear, rhi::Filter::Linear, rhi::AddressMode::Repeat);
}

void RenderContext::CreatePipeline() {
  // TODO: Load compiled SPIR-V shaders
  // For now, we'll skip shader loading and pipeline creation
  // You need to compile the shaders first

  // Load shaders - you'll need to compile .vert/.frag to .spv first
  // auto vertShader = factory_.CreateShader(...);
  // auto fragShader = factory_.CreateShader(...);

  // Pipeline layout with push constants for model matrix
  std::array<rhi::DescriptorSetLayout*, 2> layouts = {
      globalDescriptorLayout_.get(),
      materialDescriptorLayout_.get(),
  };

  std::array<rhi::PushConstantRange, 1> pushConstants = {{
      {.stage = rhi::ShaderStage::Vertex,
       .offset = 0,
       .size = sizeof(ObjectUniforms)},
  }};

  pipelineLayout_ = factory_.CreatePipelineLayout(layouts, pushConstants);

  // Graphics pipeline will be created when we have compiled shaders
  // For now, leave it null - you'll need to add shader compilation
}

void RenderContext::BeginFrame(uint32_t frameIndex) {
  currentFrame_ = frameIndex % kMaxFramesInFlight;
  auto& frame = frames_[currentFrame_];  // NOLINT

  frame.inFlightFence->Wait();
  frame.inFlightFence->Reset();
}

void RenderContext::EndFrame(uint32_t frameIndex) {
  // Frame synchronization handled by caller
}

void RenderContext::UpdateGlobalUniforms(const GlobalUniforms& uniforms) {
  auto& frame = GetCurrentFrame();
  void* data = frame.globalUniformBuffer->Map();
  std::memcpy(data, &uniforms, sizeof(GlobalUniforms));
  frame.globalUniformBuffer->Unmap();

  // Descriptor is already bound during CreateFrameResources
}
}  // namespace renderer
