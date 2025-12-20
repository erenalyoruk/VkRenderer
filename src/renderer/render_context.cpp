#include "renderer/render_context.hpp"

#include <array>
#include <cstring>

#include "logger.hpp"

#undef CreateSemaphore  // Windows macro conflict

namespace renderer {

RenderContext::RenderContext(rhi::Device& device, rhi::Factory& factory)
    : device_{device}, factory_{factory}, pipelineManager_{factory, device} {
  CreateDescriptors();

  // Initialize pipeline manager with our descriptor layouts
  pipelineManager_.Initialize(globalDescriptorLayout_.get(),
                              materialDescriptorLayout_.get());

  CreateSyncObjects();
  CreateFrameResources();
  CreateDepthBuffer();
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

    // Object uniform buffer (for dynamic per-object data if needed)
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
  // Global descriptor layout (set 0)
  std::array<rhi::DescriptorBinding, 1> globalBindings = {{
      {.binding = 0, .type = rhi::DescriptorType::UniformBuffer, .count = 1},
  }};
  globalDescriptorLayout_ = factory_.CreateDescriptorSetLayout(globalBindings);

  // Material descriptor layout (set 1)
  std::array<rhi::DescriptorBinding, 4> materialBindings = {{
      {.binding = 0, .type = rhi::DescriptorType::UniformBuffer, .count = 1},
      {.binding = 1,
       .type = rhi::DescriptorType::CombinedImageSampler,
       .count = 1},
      {.binding = 2,
       .type = rhi::DescriptorType::CombinedImageSampler,
       .count = 1},
      {.binding = 3,
       .type = rhi::DescriptorType::CombinedImageSampler,
       .count = 1},
  }};
  materialDescriptorLayout_ =
      factory_.CreateDescriptorSetLayout(materialBindings);

  defaultSampler_ = factory_.CreateSampler(
      rhi::Filter::Linear, rhi::Filter::Linear, rhi::AddressMode::Repeat);
}

void RenderContext::CreateDepthBuffer() {
  auto* swapchain = device_.GetSwapchain();
  depthTexture_ = factory_.CreateTexture(
      swapchain->GetWidth(), swapchain->GetHeight(), rhi::Format::D32Sfloat,
      rhi::TextureUsage::DepthStencilAttachment);
  LOG_INFO("Created depth buffer {}x{}", swapchain->GetWidth(),
           swapchain->GetHeight());
}

void RenderContext::BeginFrame(uint32_t frameIndex) {
  currentFrame_ = frameIndex % kMaxFramesInFlight;
  auto& frame = frames_[currentFrame_];  // NOLINT

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

void RenderContext::OnSwapchainResized() {
  // Wait for GPU to finish using old resources
  device_.WaitIdle();

  // Recreate depth buffer to match new swapchain size
  CreateDepthBuffer();

  // Recreate pipelines (swapchain format may have changed)
  pipelineManager_.RecreatePipelines();

  // Recreate semaphores if swapchain image count changed
  uint32_t imageCount = device_.GetSwapchain()->GetImageCount();
  if (imageCount != imageAvailableSemaphores_.size()) {
    imageAvailableSemaphores_.clear();
    renderFinishedSemaphores_.clear();
    CreateSyncObjects();
  }
}

}  // namespace renderer
