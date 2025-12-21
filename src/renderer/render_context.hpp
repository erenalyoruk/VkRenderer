#pragma once

#include <memory>
#include <vector>

#include <glm/glm.hpp>

#include "renderer/bindless_materials.hpp"
#include "renderer/forward_plus.hpp"
#include "renderer/gpu_culling.hpp"
#include "renderer/pipeline_manager.hpp"
#include "renderer/skybox_ibl.hpp"
#include "rhi/buffer.hpp"
#include "rhi/command.hpp"
#include "rhi/descriptor.hpp"
#include "rhi/device.hpp"
#include "rhi/factory.hpp"
#include "rhi/pipeline.hpp"
#include "rhi/sync.hpp"

namespace renderer {
constexpr uint32_t kMaxFramesInFlight = 2;

struct GlobalUniforms {
  alignas(16) glm::mat4 viewProjection;
  alignas(16) glm::mat4 view;
  alignas(16) glm::mat4 projection;
  alignas(16) glm::vec4 cameraPosition;
  alignas(16) glm::vec4 lightDirection;
  alignas(16) glm::vec4 lightColor;
  alignas(4) float lightIntensity;
  alignas(4) float time;
};

struct FrameData {
  std::unique_ptr<rhi::Fence> inFlightFence;
  std::unique_ptr<rhi::CommandPool> commandPool;
  rhi::CommandBuffer* commandBuffer{nullptr};
  std::unique_ptr<rhi::Buffer> globalUniformBuffer;
  std::unique_ptr<rhi::DescriptorSet> globalDescriptorSet;
};

class RenderContext {
 public:
  RenderContext(rhi::Device& device, rhi::Factory& factory);

  void BeginFrame(uint32_t frameIndex);
  void EndFrame(uint32_t frameIndex);

  [[nodiscard]] FrameData& GetCurrentFrame() {
    return frames_[currentFrame_];  // NOLINT
  }
  [[nodiscard]] uint32_t GetFrameIndex() const { return currentFrame_; }

  [[nodiscard]] rhi::Semaphore* GetImageAvailableSemaphore(
      uint32_t imageIndex) {
    return imageAvailableSemaphores_[imageIndex].get();
  }
  [[nodiscard]] rhi::Semaphore* GetRenderFinishedSemaphore(
      uint32_t imageIndex) {
    return renderFinishedSemaphores_[imageIndex].get();
  }

  [[nodiscard]] rhi::Device& GetDevice() { return device_; }
  [[nodiscard]] rhi::Factory& GetFactory() { return factory_; }

  [[nodiscard]] PipelineManager& GetPipelineManager() {
    return pipelineManager_;
  }
  [[nodiscard]] rhi::Pipeline* GetPipeline(PipelineType type) {
    return pipelineManager_.GetPipeline(type);
  }
  [[nodiscard]] rhi::PipelineLayout* GetPipelineLayout() {
    return pipelineManager_.GetPipelineLayout();
  }

  [[nodiscard]] rhi::Texture* GetDepthTexture() const {
    return depthTexture_.get();
  }

  [[nodiscard]] rhi::DescriptorSetLayout* GetGlobalDescriptorLayout() {
    return globalDescriptorLayout_.get();
  }

  // GPU Culling
  [[nodiscard]] GPUCulling& GetGPUCulling() { return *gpuCulling_; }

  // Bindless Materials
  [[nodiscard]] BindlessMaterialManager& GetBindlessMaterials() {
    return *bindlessMaterials_;
  }

  [[nodiscard]] SkyboxIBL& GetSkyboxIBL() { return *skyboxIBL_; }

  // Forward+ Lighting
  [[nodiscard]] ForwardPlus& GetForwardPlus() { return *forwardPlus_; }

  void UpdateGlobalUniforms(const GlobalUniforms& uniforms);
  void OnSwapchainResized();

 private:
  void CreateFrameResources();
  void CreateSyncObjects();
  void CreateDescriptors();
  void CreateDepthBuffer();

  rhi::Device& device_;
  rhi::Factory& factory_;

  std::array<FrameData, kMaxFramesInFlight> frames_;
  uint32_t currentFrame_{0};

  std::vector<std::unique_ptr<rhi::Semaphore>> imageAvailableSemaphores_;
  std::vector<std::unique_ptr<rhi::Semaphore>> renderFinishedSemaphores_;

  std::unique_ptr<rhi::DescriptorSetLayout> globalDescriptorLayout_;

  // GPU Systems
  std::unique_ptr<GPUCulling> gpuCulling_;
  std::unique_ptr<BindlessMaterialManager> bindlessMaterials_;
  std::unique_ptr<ForwardPlus> forwardPlus_;

  PipelineManager pipelineManager_;

  std::shared_ptr<rhi::Texture> depthTexture_;
  std::unique_ptr<SkyboxIBL> skyboxIBL_;
};

}  // namespace renderer
