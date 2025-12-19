#pragma once

#include <memory>

#include <entt/entt.hpp>

#include "pipelines/basic_pipeline.hpp"
#include "rendering/rendering_system.hpp"
#include "vulkan/asset_loader.hpp"
#include "vulkan/command_system.hpp"
#include "vulkan/context.hpp"
#include "vulkan/frame_manager.hpp"
#include "vulkan/global_shader_data.hpp"
#include "vulkan/material_palette.hpp"
#include "vulkan/swapchain.hpp"
#include "window.hpp"

namespace rendering {
class Renderer {
 public:
  explicit Renderer(Window& window, bool enableValidationLayers = true);
  ~Renderer();

  void RenderFrame();

  void Resize(int width, int height);

  void SetViewProjection(const glm::mat4& viewProj) {
    globalShaderData_->Update({.viewProj = viewProj});
  }

  [[nodiscard]] bool IsInitialized() const { return isInitialized_; }

  [[nodiscard]] entt::registry& GetRegistry() { return registry_; }

 private:
  Window& window_;
  bool enableValidationLayers_;

  std::unique_ptr<vulkan::Context> context_;
  std::unique_ptr<vulkan::CommandSystem> commandSystem_;
  std::unique_ptr<vulkan::Swapchain> swapchain_;
  std::unique_ptr<vulkan::FrameManager> frameManager_;
  std::unique_ptr<vulkan::GlobalShaderData> globalShaderData_;
  std::unique_ptr<vulkan::MaterialPalette> materialPalette_;
  std::unique_ptr<vulkan::AssetLoader> assetLoader_;
  std::shared_ptr<vulkan::Mesh> testMesh_;
  std::unique_ptr<BasicPipeline> basicPipeline_;

  entt::registry registry_;
  std::unique_ptr<RenderingSystem> renderingSystem_;

  bool isInitialized_{false};
};
}  // namespace rendering
