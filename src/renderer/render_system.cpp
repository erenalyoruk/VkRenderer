#include "renderer/render_system.hpp"

#include <unordered_set>

namespace renderer {

RenderSystem::RenderSystem(rhi::Device& device, rhi::Factory& factory)
    : device_{device}, factory_{factory}, context_{device, factory} {}

void RenderSystem::Render(entt::registry& registry, float deltaTime) {
  auto* swapchain = device_.GetSwapchain();

  if (swapchain->GetWidth() == 0 || swapchain->GetHeight() == 0) {
    return;
  }

  uint32_t semaphoreIndex = frameCounter_ % swapchain->GetImageCount();
  auto* imageAvailableSem = context_.GetImageAvailableSemaphore(semaphoreIndex);

  uint32_t imageIndex = swapchain->AcquireNextImage(imageAvailableSem);

  if (imageIndex == UINT32_MAX) {
    return;
  }

  context_.BeginFrame(frameCounter_);

  auto& frame = context_.GetCurrentFrame();
  auto* renderFinishedSem = context_.GetRenderFinishedSemaphore(imageIndex);

  UpdateTransforms(registry);

  // Find active camera
  auto cameraView = registry.view<ecs::CameraComponent, ecs::MainCameraTag>();
  glm::mat4 viewProjection{1.0F};
  bool hasCamera = false;
  glm::vec3 cameraPosition{0.0F};

  for (auto entity : cameraView) {
    activeCamera_ = &cameraView.get<ecs::CameraComponent>(entity);
    hasCamera = true;

    // Extract camera position from inverse view matrix
    glm::mat4 invView = glm::inverse(activeCamera_->view);
    cameraPosition = glm::vec3(invView[3]);  // Translation column
    break;
  }

  if (hasCamera) {
    GlobalUniforms globals{};
    viewProjection = activeCamera_->projection * activeCamera_->view;
    globals.viewProjection = viewProjection;
    globals.view = activeCamera_->view;
    globals.projection = activeCamera_->projection;
    globals.cameraPosition = glm::vec4(cameraPosition, 1.0F);
    globals.time = totalTime_;
    totalTime_ += deltaTime;

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

  // Build object data and run GPU culling
  if (hasCamera) {
    BuildObjectDataForCulling(registry);
    context_.GetGPUCulling().UpdateFrustum(viewProjection);
  }

  // Update material buffer if needed
  context_.GetBindlessMaterials().UpdateMaterialBuffer();

  // Execute GPU-driven rendering
  ExecuteGPUDrivenRendering(registry, imageIndex);

  auto* cmd = frame.commandBuffer;
  cmd->End();

  auto* queue = device_.GetQueue(rhi::QueueType::Graphics);

  std::array<rhi::CommandBuffer*, 1> cmdBuffers = {cmd};
  std::array<rhi::Semaphore*, 1> waitSemaphores = {imageAvailableSem};
  std::array<rhi::Semaphore*, 1> signalSemaphores = {renderFinishedSem};

  queue->Submit(cmdBuffers, waitSemaphores, signalSemaphores,
                frame.inFlightFence.get());

  std::array<rhi::Semaphore*, 1> presentWait = {renderFinishedSem};
  queue->Present(swapchain, imageIndex, presentWait);

  frameCounter_++;
}

void RenderSystem::UpdateTransforms(entt::registry& registry) {
  (void)this;

  auto rootView =
      registry.view<ecs::TransformComponent, ecs::WorldTransformComponent>(
          entt::exclude<ecs::HierarchyComponent>);

  for (auto entity : rootView) {
    auto& local = rootView.get<ecs::TransformComponent>(entity);
    auto& world = rootView.get<ecs::WorldTransformComponent>(entity);
    world.matrix = local.GetMatrix();
  }

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
  }
}

void RenderSystem::BuildObjectDataForCulling(entt::registry& registry) {
  objectDataCache_.clear();

  auto view =
      registry.view<ecs::MeshComponent, ecs::WorldTransformComponent,
                    ecs::RenderableComponent, ecs::BoundingBoxComponent>();

  for (auto entity : view) {
    auto& mesh = view.get<ecs::MeshComponent>(entity);
    auto& world = view.get<ecs::WorldTransformComponent>(entity);
    auto& bounds = view.get<ecs::BoundingBoxComponent>(entity);

    if (!mesh.vertexBuffer || !mesh.indexBuffer) {
      continue;
    }

    glm::vec3 center = bounds.GetCenter();
    glm::vec3 extents = bounds.GetExtents();
    float radius = glm::length(extents);

    for (const auto& submesh : mesh.subMeshes) {
      ObjectData objData{};
      objData.model = world.matrix;
      objData.normalMatrix = glm::transpose(glm::inverse(world.matrix));
      objData.boundingSphere = glm::vec4(center, radius);
      objData.materialIndex =
          submesh.materialIndex;  // Now references bindless material
      objData.indexCount = submesh.indexCount;
      objData.indexOffset = submesh.indexOffset;
      objData.vertexOffset = static_cast<int32_t>(submesh.vertexOffset);

      objectDataCache_.push_back(objData);
    }
  }

  context_.GetGPUCulling().UpdateObjects(objectDataCache_);
}

void RenderSystem::ExecuteGPUDrivenRendering(entt::registry& registry,
                                             uint32_t imageIndex) {
  auto& frame = context_.GetCurrentFrame();
  auto* cmd = frame.commandBuffer;

  auto* swapchain = device_.GetSwapchain();
  const auto& swapchainImages = swapchain->GetImages();
  auto* swapchainImage = swapchainImages[imageIndex];
  auto* depthTexture = context_.GetDepthTexture();

  cmd->Begin();

  // Reset draw count and execute GPU culling
  context_.GetGPUCulling().ResetDrawCount(cmd);
  context_.GetGPUCulling().Execute(cmd);

  cmd->TransitionTexture(swapchainImage, rhi::ImageLayout::Undefined,
                         rhi::ImageLayout::ColorAttachment);
  cmd->TransitionTexture(depthTexture, rhi::ImageLayout::Undefined,
                         rhi::ImageLayout::DepthStencilAttachment);

  glm::vec4 clearColor{0.1F, 0.1F, 0.15F, 1.0F};

  rhi::RenderingAttachment colorAttachment{
      .texture = swapchainImage,
      .layout = rhi::ImageLayout::ColorAttachment,
      .loadOp = rhi::LoadOp::Clear,
      .storeOp = rhi::StoreOp::Store,
      .clearValue = {clearColor.r, clearColor.g, clearColor.b, clearColor.a},
  };

  rhi::RenderingAttachment depthAttachment{
      .texture = depthTexture,
      .layout = rhi::ImageLayout::DepthStencilAttachment,
      .loadOp = rhi::LoadOp::Clear,
      .storeOp = rhi::StoreOp::DontCare,
      .clearValue = {1.0F, 0.0F, 0.0F, 0.0F},
  };

  rhi::RenderingInfo renderInfo{
      .width = swapchain->GetWidth(),
      .height = swapchain->GetHeight(),
      .colorAttachments = {&colorAttachment, 1},
      .depthAttachment = &depthAttachment,
  };

  cmd->BeginRendering(renderInfo);

  cmd->SetViewport(0, 0, static_cast<float>(swapchain->GetWidth()),
                   static_cast<float>(swapchain->GetHeight()), 0.0F, 1.0F);
  cmd->SetScissor(0, 0, swapchain->GetWidth(), swapchain->GetHeight());

  // Render geometry FIRST
  auto* pipeline = context_.GetPipeline(activePipeline_);
  if (pipeline == nullptr) {
    pipeline = context_.GetPipeline(PipelineType::PBRLit);
  }

  if (pipeline != nullptr && context_.GetGPUCulling().GetObjectCount() > 0) {
    cmd->BindPipeline(pipeline);

    // Bind descriptor sets:
    // Set 0: Global uniforms
    std::array<const rhi::DescriptorSet*, 1> globalSets = {
        frame.globalDescriptorSet.get()};
    cmd->BindDescriptorSets(pipeline, 0, globalSets);

    // Set 1: Bindless materials
    std::array<const rhi::DescriptorSet*, 1> materialSets = {
        context_.GetBindlessMaterials().GetDescriptorSet()};
    cmd->BindDescriptorSets(pipeline, 1, materialSets);

    // Set 2: Object data SSBO
    std::array<const rhi::DescriptorSet*, 1> objectSets = {
        context_.GetGPUCulling().GetObjectDescriptorSet()};
    cmd->BindDescriptorSets(pipeline, 2, objectSets);

    // Set 3: IBL
    std::array<const rhi::DescriptorSet*, 1> iblSets = {
        context_.GetSkyboxIBL().GetIBLDescriptorSet()};
    cmd->BindDescriptorSets(pipeline, 3, iblSets);

    // Bind mesh buffers and issue indirect draw
    auto view = registry.view<ecs::MeshComponent>();
    std::unordered_set<rhi::Buffer*> processedBuffers;

    for (auto entity : view) {
      auto& mesh = view.get<ecs::MeshComponent>(entity);

      if (!mesh.vertexBuffer || !mesh.indexBuffer) {
        continue;
      }

      if (processedBuffers.contains(mesh.vertexBuffer.get())) {
        continue;
      }
      processedBuffers.insert(mesh.vertexBuffer.get());

      std::array<const rhi::Buffer*, 1> vertexBuffers = {
          mesh.vertexBuffer.get()};
      std::array<uint64_t, 1> offsets = {0};
      cmd->BindVertexBuffers(0, vertexBuffers, offsets);
      cmd->BindIndexBuffer(*mesh.indexBuffer, 0, true);

      // Single indirect draw with count from GPU
      cmd->DrawIndexedIndirectCount(
          context_.GetGPUCulling().GetDrawCommandBuffer(), 0,
          context_.GetGPUCulling().GetDrawCountBuffer(), 0,
          context_.GetGPUCulling().GetMaxDrawCount(),
          sizeof(DrawIndexedIndirectCommand));
    }
  }

  // Render skybox LAST (after geometry, will be behind due to depth = 1.0)
  if (context_.GetSkyboxIBL().IsLoaded()) {
    auto* skyboxPipeline = context_.GetPipeline(PipelineType::Skybox);
    if (skyboxPipeline != nullptr) {
      cmd->BindPipeline(skyboxPipeline);

      // Bind global uniforms (Set 0)
      std::array<const rhi::DescriptorSet*, 1> globalSets = {
          frame.globalDescriptorSet.get()};
      cmd->BindDescriptorSets(skyboxPipeline, 0, globalSets);

      // Bind IBL descriptor set (Set 3)
      std::array<const rhi::DescriptorSet*, 1> iblSets = {
          context_.GetSkyboxIBL().GetIBLDescriptorSet()};
      cmd->BindDescriptorSets(skyboxPipeline, 3, iblSets);

      // Draw skybox cube
      std::array<const rhi::Buffer*, 1> vertexBuffers = {
          context_.GetSkyboxIBL().GetCubeVertexBuffer()};
      std::array<uint64_t, 1> offsets = {0};
      cmd->BindVertexBuffers(0, vertexBuffers, offsets);
      cmd->BindIndexBuffer(*context_.GetSkyboxIBL().GetCubeIndexBuffer(), 0,
                           true);
      cmd->DrawIndexed(context_.GetSkyboxIBL().GetCubeIndexCount(), 1, 0, 0, 0);
    }
  }

  cmd->EndRendering();

  cmd->TransitionTexture(swapchainImage, rhi::ImageLayout::ColorAttachment,
                         rhi::ImageLayout::Present);
}

}  // namespace renderer
