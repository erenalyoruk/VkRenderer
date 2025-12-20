#include "renderer/render_context.hpp"

#include <array>

#include "ecs/components.hpp"
#include "logger.hpp"
#include "rhi/shader_utils.hpp"

#undef CreateSemaphore  // Windows macro conflict

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
  // Load compiled shaders
  auto vertShader = rhi::CreateShaderFromFile(
      factory_, "assets/shaders/simple.vert.spv", rhi::ShaderStage::Vertex);
  auto fragShader = rhi::CreateShaderFromFile(
      factory_, "assets/shaders/simple.frag.spv", rhi::ShaderStage::Fragment);

  if (!vertShader || !fragShader) {
    LOG_ERROR("Failed to load shaders! Make sure .spv files exist.");
    return;
  }

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

  // Vertex input
  auto bindings = ecs::Vertex::GetBindings();
  auto attributes = ecs::Vertex::GetAttributes();

  // Get swapchain format
  auto* swapchain = device_.GetSwapchain();
  rhi::Format colorFormat = swapchain->GetImages()[0]->GetFormat();

  std::array<rhi::Format, 1> colorFormats = {colorFormat};

  rhi::GraphicsPipelineDesc pipelineDesc{
      .vertexShader = vertShader.get(),
      .fragmentShader = fragShader.get(),
      .layout = pipelineLayout_.get(),
      .vertexBindings = bindings,
      .vertexAttributes = attributes,
      .colorFormats = colorFormats,
      .depthFormat = rhi::Format::D32Sfloat,
  };

  pipeline_ = factory_.CreateGraphicsPipeline(pipelineDesc);

  if (!pipeline_) {
    LOG_ERROR("Failed to create graphics pipeline!");
  } else {
    LOG_INFO("Graphics pipeline created successfully.");
  }
}

void RenderContext::BeginFrame(uint32_t frameIndex) {
  currentFrame_ = frameIndex % kMaxFramesInFlight;
  auto& frame = frames_[currentFrame_];  // NOLINT

  frame.inFlightFence->Wait();
  frame.inFlightFence->Reset();
}

void RenderContext::EndFrame(uint32_t /*frameIndex*/) {
  // Frame synchronization handled by caller
}

void RenderContext::UpdateGlobalUniforms(const GlobalUniforms& uniforms) {
  auto& frame = GetCurrentFrame();
  void* data = frame.globalUniformBuffer->Map();
  std::memcpy(data, &uniforms, sizeof(GlobalUniforms));
  frame.globalUniformBuffer->Unmap();
}

}  // namespace renderer
