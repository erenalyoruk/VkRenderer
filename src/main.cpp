#include <entt/entt.hpp>

#include "application.hpp"
#include "camera/camera_controller.hpp"
#include "camera/fps_camera_controller.hpp"
#include "ecs/components.hpp"
#include "logger.hpp"
#include "renderer/pipeline_manager.hpp"
#include "renderer/render_system.hpp"
#include "resource/resource_manager.hpp"
#include "resource/scene_loader.hpp"
#include "rhi/backend.hpp"

int main() {
  quill::Backend::start();

  GetLogger()->set_log_level(
#ifndef NDEBUG
      quill::LogLevel::Debug
#else
      quill::LogLevel::Info
#endif
  );

  // Create application window
  Application app(1920, 1080, "Vulkan Renderer - Sponza");

  // Create RHI backend
  auto [device, factory]{rhi::BackendFactory::Create(
      rhi::BackendType::Vulkan, app.GetWindow(), app.GetWindow().GetWidth(),
      app.GetWindow().GetHeight(), true)};

  // Create ECS registry
  entt::registry registry;

  // Create resource manager and load Sponza
  resource::ResourceManager resources{*factory};

  // Create render system
  renderer::RenderSystem renderSystem{*device, *factory};

  // Current pipeline mode
  renderer::PipelineType currentPipeline = renderer::PipelineType::PBRLit;

  resource::Model* sponzaModel =
      resources.LoadModel("assets/models/Sponza/Sponza.gltf");
  if (sponzaModel == nullptr) {
    LOG_WARNING("Sponza model not found at assets/models/Sponza/Sponza.gltf");
    return 1;
  }

  LOG_INFO("Sponza model loaded successfully");

  ecs::TransformComponent sponzaTransform{};
  sponzaTransform.scale = glm::vec3(1.0F);
  auto rootEntity = resource::InstantiateModel(
      registry, *sponzaModel, renderSystem.GetContext().GetBindlessMaterials());

  size_t totalPrimitives = 0;
  for (const auto& mesh : sponzaModel->meshes) {
    totalPrimitives += mesh.primitives.size();
  }
  LOG_INFO("Sponza: {} meshes, {} primitives, {} materials, {} textures",
           sponzaModel->meshes.size(), totalPrimitives,
           sponzaModel->materials.size(), sponzaModel->textures.size());

  // Create camera entity
  auto cameraEntity{registry.create()};
  ecs::CameraComponent cameraComp{};
  registry.emplace<ecs::CameraComponent>(cameraEntity, cameraComp);
  registry.emplace<ecs::MainCameraTag>(cameraEntity);

  // Create directional light
  auto lightEntity = registry.create();
  ecs::DirectionalLightComponent light{
      .direction = glm::normalize(glm::vec3(-1.0F, -1.0F, -0.5F)),
      .color = glm::vec3(1.0F, 0.98F, 0.95F),
      .intensity = 1.5F,
  };
  registry.emplace<ecs::DirectionalLightComponent>(lightEntity, light);

  // Create camera controller
  camera::CameraSettings cameraSettings{
      .fov = glm::radians(60.0F),
      .nearPlane = 0.1F,
      .farPlane = 1000.0F,
      .movementSpeed = 5.0F,
      .mouseSensitivity = 0.1F,
  };
  camera::Camera camera{cameraSettings, app.GetWindow().GetAspectRatio()};
  camera.SetPosition(glm::vec3(0.0F, 2.0F, 5.0F));
  camera::FPSCameraController cameraController{camera};

  // Handle window resize
  app.GetWindow().AddResizeCallback(
      [&renderSystem, &camera, &device](int width, int height) {
        camera.SetAspectRatio(static_cast<float>(width) /
                              static_cast<float>(height));
        device->GetSwapchain()->Resize(width, height);
        renderSystem.OnSwapchainResized();
      });

  // Accumulator for stats logging
  float statsTimer = 0.0F;

  LOG_INFO("Controls: 1=PBR Lit, 2=Unlit, 3=Wireframe, WASD=Move, Mouse=Look");

  // Main loop
  app.Run(
      // Update callback
      [&](float deltaTime) {
        auto& input = app.GetInput();

        cameraController.Update(input, deltaTime);

        if (input.IsKeyPressed(input::ScanCode::Key1)) {
          currentPipeline = renderer::PipelineType::PBRLit;
          renderSystem.SetActivePipeline(currentPipeline);
          LOG_INFO("Switched to PBR Lit pipeline");
        }

        if (input.IsKeyPressed(input::ScanCode::Key2)) {
          currentPipeline = renderer::PipelineType::Unlit;
          renderSystem.SetActivePipeline(currentPipeline);
          LOG_INFO("Switched to Unlit pipeline");
        }

        if (input.IsKeyPressed(input::ScanCode::Key3)) {
          currentPipeline = renderer::PipelineType::Wireframe;
          renderSystem.SetActivePipeline(currentPipeline);
          LOG_INFO("Switched to Wireframe pipeline");
        }

        // Sync camera data to ECS camera component
        auto& camComp = registry.get<ecs::CameraComponent>(cameraEntity);
        camComp.view = camera.GetView();
        camComp.projection = camera.GetProjection();
        camComp.frustumPlanes = camera.GetFrustumPlanes();
      },
      // Render callback
      [&](float deltaTime) { renderSystem.Render(registry, deltaTime); });

  device->WaitIdle();
  return 0;
}
