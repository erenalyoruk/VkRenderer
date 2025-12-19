#pragma once

#include <memory>

#include "camera.hpp"
#include "vulkan/asset_loader.hpp"
#include "vulkan/command_system.hpp"
#include "vulkan/context.hpp"
#include "vulkan/descriptors.hpp"
#include "vulkan/frame_manager.hpp"
#include "vulkan/global_shader_data.hpp"
#include "vulkan/material_palette.hpp"
#include "vulkan/swapchain.hpp"
#include "window.hpp"

namespace vulkan {
class Renderer {
 public:
  explicit Renderer(Window& window, bool enableValidationLayers = true);
  ~Renderer();

  void RenderFrame();

  void Resize(int width, int height);

  [[nodiscard]] bool IsInitialized() const { return isInitialized_; }

 private:
  Window& window_;
  bool enableValidationLayers_;

  std::unique_ptr<Context> context_;
  std::unique_ptr<CommandSystem> commandSystem_;
  std::unique_ptr<Swapchain> swapchain_;
  std::unique_ptr<FrameManager> frameManager_;
  std::unique_ptr<GlobalShaderData> globalShaderData_;
  std::unique_ptr<MaterialPalette> materialPalette_;
  std::unique_ptr<Camera> camera_;
  std::unique_ptr<AssetLoader> assetLoader_;
  std::unique_ptr<vk::UniqueShaderModule> vertShader_;
  std::unique_ptr<vk::UniqueShaderModule> fragShader_;
  std::unique_ptr<DescriptorAllocator> descriptorAllocator_;
  vk::UniqueDescriptorSetLayout descriptorSetLayout_;
  vk::UniquePipelineLayout pipelineLayout_;
  vk::DescriptorSet descriptorSet_;
  vk::UniquePipeline pipeline_;
  std::unique_ptr<Mesh> testMesh_;

  bool isInitialized_{false};
};
}  // namespace vulkan
