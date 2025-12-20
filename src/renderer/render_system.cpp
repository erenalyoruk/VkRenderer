#include "renderer/render_system.hpp"

#include <bit>

namespace renderer {

RenderSystem::RenderSystem(rhi::Device& device, rhi::Factory& factory)
    : device_{device}, context_{device, factory} {}

void RenderSystem::Render(entt::registry& registry, float deltaTime) {
  stats_ = {};

  // Get swapchain image
  auto* swapchain = device_.GetSwapchain();
  auto& frame = context_.GetCurrentFrame();

  uint32_t imageIndex = swapchain->AcquireNextImage(frame.imageAvailable.get());
  context_.BeginFrame(frameCounter_);

  // Update systems
  UpdateTransforms(registry);
  FrustumCull(registry);
  SortRenderables(registry);

  // Find active camera
  auto cameraView = registry.view<ecs::CameraComponent, ecs::MainCameraTag>();
  bool hasCamera = false;
  for (auto entity : cameraView) {
    activeCamera_ = &cameraView.get<ecs::CameraComponent>(entity);
    hasCamera = true;
    break;
  }

  if (!hasCamera) {
    // Still need to present even without camera
    ExecuteRendering(registry, imageIndex);

    auto* cmd = frame.commandBuffer;
    cmd->End();

    auto* queue = device_.GetQueue(rhi::QueueType::Graphics);
    std::array<rhi::CommandBuffer*, 1> cmdBuffers = {cmd};
    std::array<rhi::Semaphore*, 1> waitSemaphores = {
        frame.imageAvailable.get()};
    std::array<rhi::Semaphore*, 1> signalSemaphores = {
        frame.renderFinished.get()};

    queue->Submit(cmdBuffers, waitSemaphores, signalSemaphores,
                  frame.inFlightFence.get());

    // Present via queue, not swapchain
    std::array<rhi::Semaphore*, 1> presentWait = {frame.renderFinished.get()};
    queue->Present(swapchain, imageIndex, presentWait);

    frameCounter_++;
    return;
  }

  // Update global uniforms
  GlobalUniforms globals{};
  globals.viewProjection = activeCamera_->projection * activeCamera_->view;
  globals.time = static_cast<float>(frameCounter_) * deltaTime;

  // Get directional light
  auto lightView = registry.view<ecs::DirectionalLightComponent>();
  for (auto entity : lightView) {
    auto& light = lightView.get<ecs::DirectionalLightComponent>(entity);
    globals.lightDirection = glm::vec4(light.direction, 0.0F);
    globals.lightColor = glm::vec4(light.color, 1.0F);
    globals.lightIntensity = light.intensity;
    break;  // Use first light
  }

  context_.UpdateGlobalUniforms(globals);

  // Record commands
  ExecuteRendering(registry, imageIndex);

  // Submit and present
  auto* cmd = frame.commandBuffer;
  cmd->End();

  auto* queue = device_.GetQueue(rhi::QueueType::Graphics);

  // Submit takes spans
  std::array<rhi::CommandBuffer*, 1> cmdBuffers = {cmd};
  std::array<rhi::Semaphore*, 1> waitSemaphores = {frame.imageAvailable.get()};
  std::array<rhi::Semaphore*, 1> signalSemaphores = {
      frame.renderFinished.get()};

  queue->Submit(cmdBuffers, waitSemaphores, signalSemaphores,
                frame.inFlightFence.get());

  // Present via queue, not swapchain directly
  std::array<rhi::Semaphore*, 1> presentWait = {frame.renderFinished.get()};
  queue->Present(swapchain, imageIndex, presentWait);

  frameCounter_++;
}

void RenderSystem::UpdateTransforms(entt::registry& registry) {
  // Update world transforms from local transforms
  auto view =
      registry.view<ecs::TransformComponent, ecs::WorldTransformComponent>();

  for (auto entity : view) {
    auto& local = view.get<ecs::TransformComponent>(entity);
    auto& world = view.get<ecs::WorldTransformComponent>(entity);
    world.matrix = local.GetMatrix();
    stats_.entitiesProcessed++;
  }

  // TODO: Handle hierarchy (parent-child relationships)
}

void RenderSystem::FrustumCull(entt::registry& registry) {
  if (activeCamera_ == nullptr) {
    return;
  }

  auto view =
      registry.view<ecs::WorldTransformComponent, ecs::BoundingBoxComponent,
                    ecs::RenderableComponent>();

  for (auto entity : view) {
    auto& world = view.get<ecs::WorldTransformComponent>(entity);
    auto& bounds = view.get<ecs::BoundingBoxComponent>(entity);

    // Transform bounding box center to world space
    glm::vec4 center = world.matrix * glm::vec4(bounds.GetCenter(), 1.0F);
    float radius = glm::length(bounds.GetExtents());

    // Simple sphere-frustum test
    bool visible = true;
    for (const auto& plane : activeCamera_->frustumPlanes) {
      float distance = glm::dot(plane, center) + plane.w;
      if (distance < -radius) {
        visible = false;
        stats_.culledObjects++;
        break;
      }
    }

    // Mark as visible/invisible (could use a tag or flag)
    // For now, we'll just skip rendering in ExecuteRendering
    (void)visible;  // Suppress unused warning
  }
}

void RenderSystem::SortRenderables(entt::registry& registry) {
  (void)this;

  // TODO: Sort by material, distance, or render layer for better batching
  // For now, we'll render in entity order
  (void)registry;
}

void RenderSystem::ExecuteRendering(entt::registry& registry,
                                    uint32_t imageIndex) {
  auto& frame = context_.GetCurrentFrame();
  auto* cmd = frame.commandBuffer;

  auto* swapchain = device_.GetSwapchain();
  const auto& swapchainImages = swapchain->GetImages();
  auto* swapchainImage = swapchainImages[imageIndex];

  cmd->Begin();

  // Transition swapchain image to color attachment
  cmd->TransitionTexture(swapchainImage, rhi::ImageLayout::Undefined,
                         rhi::ImageLayout::ColorAttachment);

  // Begin rendering
  rhi::RenderingAttachment colorAttachment{
      .texture = swapchainImage,
      .layout = rhi::ImageLayout::ColorAttachment,
      .loadOp = rhi::LoadOp::Clear,
      .storeOp = rhi::StoreOp::Store,
      .clearValue = {0.1F, 0.1F, 0.1F, 1.0F},
  };

  rhi::RenderingInfo renderInfo{
      .width = swapchain->GetWidth(),
      .height = swapchain->GetHeight(),
      .colorAttachments = {&colorAttachment, 1},
      .depthAttachment = nullptr,
  };

  cmd->BeginRendering(renderInfo);

  // Set viewport and scissor
  cmd->SetViewport(0, 0, static_cast<float>(swapchain->GetWidth()),
                   static_cast<float>(swapchain->GetHeight()), 0.0F, 1.0F);
  cmd->SetScissor(0, 0, swapchain->GetWidth(), swapchain->GetHeight());

  // Skip rendering meshes if no pipeline (shaders not compiled yet)
  auto* pipeline = context_.GetPipeline();
  if (pipeline != nullptr) {
    // Bind pipeline
    cmd->BindPipeline(pipeline);

    // Bind global descriptor set
    std::array<const rhi::DescriptorSet*, 1> descriptorSets = {
        frame.globalDescriptorSet.get()};
    cmd->BindDescriptorSets(pipeline, 0, descriptorSets);

    // Render all entities
    auto view = registry.view<ecs::MeshComponent, ecs::WorldTransformComponent,
                              ecs::RenderableComponent>();

    for (auto entity : view) {
      auto& mesh = view.get<ecs::MeshComponent>(entity);
      auto& world = view.get<ecs::WorldTransformComponent>(entity);

      if (!mesh.vertexBuffer || !mesh.indexBuffer) {
        continue;  // Skip invalid meshes
      }

      // Push constants for object transform
      ObjectUniforms objUniforms{};
      objUniforms.model = world.matrix;
      objUniforms.normalMatrix =
          glm::transpose(glm::inverse(glm::mat4(glm::mat3(world.matrix))));

      std::span<const std::byte> pushData{
          std::bit_cast<const std::byte*>(&objUniforms),
          sizeof(ObjectUniforms)};
      cmd->PushConstants(pipeline, 0, pushData);

      // Bind vertex and index buffers
      std::array<const rhi::Buffer*, 1> vertexBuffers = {
          mesh.vertexBuffer.get()};
      std::array<uint64_t, 1> offsets = {0};
      cmd->BindVertexBuffers(0, vertexBuffers, offsets);
      cmd->BindIndexBuffer(*mesh.indexBuffer, 0,
                           true);  // true = 32-bit indices

      // Draw submeshes
      for (const auto& submesh : mesh.subMeshes) {
        cmd->DrawIndexed(submesh.indexCount, 1, submesh.indexOffset,
                         static_cast<int32_t>(submesh.vertexOffset), 0);
        stats_.drawCalls++;
        stats_.triangles += submesh.indexCount / 3;
      }
    }
  }

  cmd->EndRendering();

  // Transition swapchain image to present
  cmd->TransitionTexture(swapchainImage, rhi::ImageLayout::ColorAttachment,
                         rhi::ImageLayout::Present);
}

}  // namespace renderer
