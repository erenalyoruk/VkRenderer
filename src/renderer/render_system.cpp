#include "renderer/render_system.hpp"

#include <bit>

namespace renderer {

RenderSystem::RenderSystem(rhi::Device& device, rhi::Factory& factory)
    : device_{device}, context_{device, factory} {}

void RenderSystem::Render(entt::registry& registry, float deltaTime) {
  stats_ = {};

  auto* swapchain = device_.GetSwapchain();

  // Begin frame (waits for fence, resets command pool)
  context_.BeginFrame(frameCounter_);

  auto& frame = context_.GetCurrentFrame();

  // Use semaphore indexed by current frame for acquire
  // We'll get the actual image index back
  uint32_t semaphoreIndex = frameCounter_ % swapchain->GetImageCount();
  auto* imageAvailableSem = context_.GetImageAvailableSemaphore(semaphoreIndex);

  uint32_t imageIndex = swapchain->AcquireNextImage(imageAvailableSem);

  // Use semaphore indexed by acquired image for rendering/present
  auto* renderFinishedSem = context_.GetRenderFinishedSemaphore(imageIndex);

  // Update systems
  UpdateTransforms(registry);

  // Find active camera
  auto cameraView = registry.view<ecs::CameraComponent, ecs::MainCameraTag>();
  bool hasCamera = false;
  for (auto entity : cameraView) {
    activeCamera_ = &cameraView.get<ecs::CameraComponent>(entity);
    hasCamera = true;
    break;
  }

  // Update global uniforms if we have a camera
  if (hasCamera) {
    GlobalUniforms globals{};
    globals.viewProjection = activeCamera_->projection * activeCamera_->view;
    globals.time = static_cast<float>(frameCounter_) * deltaTime;

    auto lightView = registry.view<ecs::DirectionalLightComponent>();
    for (auto entity : lightView) {
      auto& light = lightView.get<ecs::DirectionalLightComponent>(entity);
      globals.lightDirection = glm::vec4(light.direction, 0.0F);
      globals.lightColor = glm::vec4(light.color, 1.0F);
      globals.lightIntensity = light.intensity;
      break;
    }

    context_.UpdateGlobalUniforms(globals);
  }

  // Record commands
  ExecuteRendering(registry, imageIndex);

  // Submit
  auto* cmd = frame.commandBuffer;
  cmd->End();

  auto* queue = device_.GetQueue(rhi::QueueType::Graphics);

  std::array<rhi::CommandBuffer*, 1> cmdBuffers = {cmd};
  std::array<rhi::Semaphore*, 1> waitSemaphores = {imageAvailableSem};
  std::array<rhi::Semaphore*, 1> signalSemaphores = {renderFinishedSem};

  queue->Submit(cmdBuffers, waitSemaphores, signalSemaphores,
                frame.inFlightFence.get());

  // Present
  std::array<rhi::Semaphore*, 1> presentWait = {renderFinishedSem};
  queue->Present(swapchain, imageIndex, presentWait);

  frameCounter_++;
}

void RenderSystem::UpdateTransforms(entt::registry& registry) {
  // First, update root transforms (no parent)
  auto rootView =
      registry.view<ecs::TransformComponent, ecs::WorldTransformComponent>(
          entt::exclude<ecs::HierarchyComponent>);

  for (auto entity : rootView) {
    auto& local = rootView.get<ecs::TransformComponent>(entity);
    auto& world = rootView.get<ecs::WorldTransformComponent>(entity);
    world.matrix = local.GetMatrix();
    stats_.entitiesProcessed++;
  }

  // Then update children with parent hierarchy
  auto childView =
      registry.view<ecs::TransformComponent, ecs::WorldTransformComponent,
                    ecs::HierarchyComponent>();

  for (auto entity : childView) {
    auto& local = childView.get<ecs::TransformComponent>(entity);
    auto& world = childView.get<ecs::WorldTransformComponent>(entity);
    auto& hierarchy = childView.get<ecs::HierarchyComponent>(entity);

    if (hierarchy.parent != entt::null &&
        registry.all_of<ecs::WorldTransformComponent>(hierarchy.parent)) {
      auto& parentWorld =
          registry.get<ecs::WorldTransformComponent>(hierarchy.parent);
      world.matrix = parentWorld.matrix * local.GetMatrix();
    } else {
      world.matrix = local.GetMatrix();
    }
    stats_.entitiesProcessed++;
  }
}

void RenderSystem::FrustumCull(entt::registry& /*registry*/) {}

void RenderSystem::SortRenderables(entt::registry& /*registry*/) {}

void RenderSystem::ExecuteRendering(entt::registry& registry,
                                    uint32_t imageIndex) {
  auto& frame = context_.GetCurrentFrame();
  auto* cmd = frame.commandBuffer;

  auto* swapchain = device_.GetSwapchain();
  const auto& swapchainImages = swapchain->GetImages();
  auto* swapchainImage = swapchainImages[imageIndex];
  auto* depthTexture = context_.GetDepthTexture();  // Add this

  cmd->Begin();

  cmd->TransitionTexture(swapchainImage, rhi::ImageLayout::Undefined,
                         rhi::ImageLayout::ColorAttachment);

  // Transition depth buffer
  cmd->TransitionTexture(depthTexture, rhi::ImageLayout::Undefined,
                         rhi::ImageLayout::DepthStencilAttachment);

  rhi::RenderingAttachment colorAttachment{
      .texture = swapchainImage,
      .layout = rhi::ImageLayout::ColorAttachment,
      .loadOp = rhi::LoadOp::Clear,
      .storeOp = rhi::StoreOp::Store,
      .clearValue = {0.2F, 0.3F, 0.4F, 1.0F},
  };

  // Add depth attachment
  rhi::RenderingAttachment depthAttachment{
      .texture = depthTexture,
      .layout = rhi::ImageLayout::DepthStencilAttachment,
      .loadOp = rhi::LoadOp::Clear,
      .storeOp = rhi::StoreOp::DontCare,
      .clearValue = {1.0F, 0.0F, 0.0F, 0.0F},  // Clear to 1.0 for depth
  };

  rhi::RenderingInfo renderInfo{
      .width = swapchain->GetWidth(),
      .height = swapchain->GetHeight(),
      .colorAttachments = {&colorAttachment, 1},
      .depthAttachment = &depthAttachment,  // Use depth buffer!
  };

  cmd->BeginRendering(renderInfo);

  cmd->SetViewport(0, 0, static_cast<float>(swapchain->GetWidth()),
                   static_cast<float>(swapchain->GetHeight()), 0.0F, 1.0F);
  cmd->SetScissor(0, 0, swapchain->GetWidth(), swapchain->GetHeight());

  auto* pipeline = context_.GetPipeline();
  if (pipeline != nullptr) {
    cmd->BindPipeline(pipeline);

    std::array<const rhi::DescriptorSet*, 1> descriptorSets = {
        frame.globalDescriptorSet.get()};
    cmd->BindDescriptorSets(pipeline, 0, descriptorSets);

    auto view = registry.view<ecs::MeshComponent, ecs::WorldTransformComponent,
                              ecs::RenderableComponent>();

    for (auto entity : view) {
      auto& mesh = view.get<ecs::MeshComponent>(entity);
      auto& world = view.get<ecs::WorldTransformComponent>(entity);

      if (!mesh.vertexBuffer || !mesh.indexBuffer) {
        continue;
      }

      ObjectUniforms objUniforms{};
      objUniforms.model = world.matrix;
      objUniforms.normalMatrix =
          glm::transpose(glm::inverse(glm::mat4(glm::mat3(world.matrix))));

      std::span<const std::byte> pushData{
          std::bit_cast<const std::byte*>(&objUniforms),
          sizeof(ObjectUniforms)};
      cmd->PushConstants(pipeline, 0, pushData);

      std::array<const rhi::Buffer*, 1> vertexBuffers = {
          mesh.vertexBuffer.get()};
      std::array<uint64_t, 1> offsets = {0};
      cmd->BindVertexBuffers(0, vertexBuffers, offsets);
      cmd->BindIndexBuffer(*mesh.indexBuffer, 0, true);

      for (const auto& submesh : mesh.subMeshes) {
        cmd->DrawIndexed(submesh.indexCount, 1, submesh.indexOffset,
                         static_cast<int32_t>(submesh.vertexOffset), 0);
        stats_.drawCalls++;
        stats_.triangles += submesh.indexCount / 3;
      }
    }
  }

  cmd->EndRendering();

  cmd->TransitionTexture(swapchainImage, rhi::ImageLayout::ColorAttachment,
                         rhi::ImageLayout::Present);
}

}  // namespace renderer
