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
  CreateSyncObjects();
  CreateFrameResources();
}

void RenderContext::CreateSyncObjects() {
  // Create semaphores per swapchain image
  uint32_t imageCount = device_.GetSwapchain()->GetImageCount();

  imageAvailableSemaphores_.reserve(imageCount);
  renderFinishedSemaphores_.reserve(imageCount);

  for (uint32_t i = 0; i < imageCount; ++i) {
    imageAvailableSemaphores_.push_back(factory_.CreateSemaphore());
    renderFinishedSemaphores_.push_back(factory_.CreateSemaphore());
  }

  LOG_DEBUG("Created {} semaphore pairs for swapchain images", imageCount);
}

void RenderContext::CreateFrameResources() {
  for (auto& frame : frames_) {
    frame.inFlightFence = factory_.CreateFence(true);

    frame.commandPool = factory_.CreateCommandPool(rhi::QueueType::Graphics);
    frame.commandBuffer = frame.commandPool->AllocateCommandBuffer();

    // Global uniform buffer
    frame.globalUniformBuffer =
        factory_.CreateBuffer(sizeof(GlobalUniforms), rhi::BufferUsage::Uniform,
                              rhi::MemoryUsage::CPUToGPU);

    // Object uniform buffer
    frame.objectUniformBuffer = factory_.CreateBuffer(
        sizeof(ObjectUniforms) * 10000, rhi::BufferUsage::Uniform,
        rhi::MemoryUsage::CPUToGPU);

    // Global descriptor set
    frame.globalDescriptorSet =
        factory_.CreateDescriptorSet(globalDescriptorLayout_.get());

    frame.globalDescriptorSet->BindBuffer(0, frame.globalUniformBuffer.get(), 0,
                                          sizeof(GlobalUniforms));
  }
}

void RenderContext::CreateDescriptors() {
  std::array<rhi::DescriptorBinding, 1> globalBindings = {{
      {.binding = 0, .type = rhi::DescriptorType::UniformBuffer, .count = 1},
  }};
  globalDescriptorLayout_ = factory_.CreateDescriptorSetLayout(globalBindings);

  std::array<rhi::DescriptorBinding, 4> materialBindings = {{
      {.binding = 0, .type = rhi::DescriptorType::UniformBuffer, .count = 1},
      {.binding = 1, .type = rhi::DescriptorType::SampledImage, .count = 1},
      {.binding = 2, .type = rhi::DescriptorType::SampledImage, .count = 1},
      {.binding = 3, .type = rhi::DescriptorType::SampledImage, .count = 1},
  }};
  materialDescriptorLayout_ =
      factory_.CreateDescriptorSetLayout(materialBindings);

  defaultSampler_ = factory_.CreateSampler(
      rhi::Filter::Linear, rhi::Filter::Linear, rhi::AddressMode::Repeat);
}

void RenderContext::CreatePipeline() {
  auto vertShader = rhi::CreateShaderFromFile(
      factory_, "assets/shaders/simple.vert.spv", rhi::ShaderStage::Vertex);
  auto fragShader = rhi::CreateShaderFromFile(
      factory_, "assets/shaders/simple.frag.spv", rhi::ShaderStage::Fragment);

  if (!vertShader || !fragShader) {
    LOG_ERROR("Failed to load shaders! Make sure .spv files exist.");
    return;
  }

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

  auto bindings = ecs::Vertex::GetBindings();
  auto attributes = ecs::Vertex::GetAttributes();

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
  auto& frame = frames_[currentFrame_]; // NOLINT

  frame.inFlightFence->Wait();
  frame.inFlightFence->Reset();
  frame.commandPool->Reset();
}

void RenderContext::EndFrame(uint32_t /*frameIndex*/) {}

void RenderContext::UpdateGlobalUniforms(const GlobalUniforms& uniforms) {
  auto& frame = GetCurrentFrame();
  void* data = frame.globalUniformBuffer->Map();
  std::memcpy(data, &uniforms, sizeof(GlobalUniforms));
  frame.globalUniformBuffer->Unmap();
}

}  // namespace renderer
