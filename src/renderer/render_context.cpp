#include "renderer/render_context.hpp"

#include <array>
#include <cstring>

#include "logger.hpp"

#undef CreateSemaphore  // Windows macro conflict

namespace renderer {

RenderContext::RenderContext(rhi::Device& device, rhi::Factory& factory)
    : device_{device}, factory_{factory}, pipelineManager_{factory, device} {
  CreateDescriptors();

  // Initialize bindless material manager
  bindlessMaterials_ = std::make_unique<BindlessMaterialManager>(factory_);
  bindlessMaterials_->Initialize();

  // Initialize GPU culling
  gpuCulling_ = std::make_unique<GPUCulling>(factory_, device_);
  gpuCulling_->Initialize();

  // Initialize pipeline manager with descriptor layouts:
  // set 0: global uniforms
  // set 1: bindless materials
  // set 2: object data SSBO
  pipelineManager_.Initialize(globalDescriptorLayout_.get(),
                              bindlessMaterials_->GetDescriptorLayout(),
                              gpuCulling_->GetObjectDescriptorLayout());

  CreateSyncObjects();
  CreateFrameResources();
  CreateDepthBuffer();
}

void RenderContext::CreateSyncObjects() {
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

    frame.globalUniformBuffer =
        factory_.CreateBuffer(sizeof(GlobalUniforms), rhi::BufferUsage::Uniform,
                              rhi::MemoryUsage::CPUToGPU);

    frame.globalDescriptorSet =
        factory_.CreateDescriptorSet(globalDescriptorLayout_.get());

    frame.globalDescriptorSet->BindBuffer(0, frame.globalUniformBuffer.get(), 0,
                                          sizeof(GlobalUniforms));
  }
}

void RenderContext::CreateDescriptors() {
  // Global descriptor layout (set 0) - just camera/lighting uniforms
  std::array<rhi::DescriptorBinding, 1> globalBindings = {{
      {.binding = 0, .type = rhi::DescriptorType::UniformBuffer, .count = 1},
  }};
  globalDescriptorLayout_ = factory_.CreateDescriptorSetLayout(globalBindings);
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
  device_.WaitIdle();

  CreateDepthBuffer();
  pipelineManager_.RecreatePipelines();

  uint32_t imageCount = device_.GetSwapchain()->GetImageCount();
  if (imageCount != imageAvailableSemaphores_.size()) {
    imageAvailableSemaphores_.clear();
    renderFinishedSemaphores_.clear();
    CreateSyncObjects();
  }
}

}  // namespace renderer
