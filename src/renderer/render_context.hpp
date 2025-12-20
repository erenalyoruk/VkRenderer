#pragma once

#include <memory>
#include <vector>

#include <glm/glm.hpp>

#include "rhi/buffer.hpp"
#include "rhi/command.hpp"
#include "rhi/descriptor.hpp"
#include "rhi/device.hpp"
#include "rhi/factory.hpp"
#include "rhi/pipeline.hpp"
#include "rhi/sampler.hpp"
#include "rhi/sync.hpp"

namespace renderer {
constexpr uint32_t kMaxFramesInFlight = 2;

struct GlobalUniforms {
  alignas(16) glm::mat4 viewProjection;
  alignas(16) glm::vec4 cameraPosition;
  alignas(16) glm::vec4 lightDirection;
  alignas(16) glm::vec4 lightColor;
  alignas(4) float lightIntensity;
  alignas(4) float time;
};

struct ObjectUniforms {
  alignas(16) glm::mat4 model;
  alignas(16) glm::mat4 normalMatrix;
};

struct MaterialUniforms {
  alignas(16) glm::vec4 baseColor;
  alignas(4) float metallic;
  alignas(4) float roughness;
};

struct FrameData {
  // Synchronization - per frame in flight
  std::unique_ptr<rhi::Fence> inFlightFence;

  // Command buffers
  std::unique_ptr<rhi::CommandPool> commandPool;
  rhi::CommandBuffer* commandBuffer{nullptr};

  // Uniform buffers
  std::unique_ptr<rhi::Buffer> globalUniformBuffer;
  std::unique_ptr<rhi::Buffer> objectUniformBuffer;

  // Descriptor sets
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

  // Get semaphores indexed by swapchain image
  [[nodiscard]] rhi::Semaphore* GetImageAvailableSemaphore(
      uint32_t imageIndex) {
    return imageAvailableSemaphores_[imageIndex].get();
  }
  [[nodiscard]] rhi::Semaphore* GetRenderFinishedSemaphore(
      uint32_t imageIndex) {
    return renderFinishedSemaphores_[imageIndex].get();
  }

  // Resource access
  [[nodiscard]] rhi::Device& GetDevice() { return device_; }
  [[nodiscard]] rhi::Factory& GetFactory() { return factory_; }
  [[nodiscard]] rhi::Pipeline* GetPipeline() { return pipeline_.get(); }
  [[nodiscard]] rhi::PipelineLayout* GetPipelineLayout() {
    return pipelineLayout_.get();
  }
  [[nodiscard]] rhi::Sampler& GetDefaultSampler() { return *defaultSampler_; }

  [[nodiscard]] rhi::Texture* GetDepthTexture() const {
    return depthTexture_.get();
  }

  void UpdateGlobalUniforms(const GlobalUniforms& uniforms);

  void OnSwapchainResized();

 private:
  void CreateFrameResources();
  void CreateSyncObjects();
  void CreatePipeline();
  void CreateDescriptors();
  void CreateDepthBuffer();

  rhi::Device& device_;
  rhi::Factory& factory_;

  std::array<FrameData, kMaxFramesInFlight> frames_;
  uint32_t currentFrame_{0};

  // Semaphores per swapchain image (not per frame!)
  std::vector<std::unique_ptr<rhi::Semaphore>> imageAvailableSemaphores_;
  std::vector<std::unique_ptr<rhi::Semaphore>> renderFinishedSemaphores_;

  // Shared resources
  std::unique_ptr<rhi::DescriptorSetLayout> globalDescriptorLayout_;
  std::unique_ptr<rhi::DescriptorSetLayout> materialDescriptorLayout_;
  std::unique_ptr<rhi::PipelineLayout> pipelineLayout_;
  std::unique_ptr<rhi::Pipeline> pipeline_;
  std::unique_ptr<rhi::Sampler> defaultSampler_;
  std::shared_ptr<rhi::Texture> depthTexture_;
};
}  // namespace renderer
