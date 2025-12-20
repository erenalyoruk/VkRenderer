#include <entt/entt.hpp>

#include "application.hpp"
#include "camera/camera_controller.hpp"
#include "camera/fps_camera_controller.hpp"
#include "ecs/components.hpp"
#include "logger.hpp"
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

  // Try different possible paths for Sponza
  resource::Model* sponzaModel =
      resources.LoadModel("assets/models/Sponza/Sponza.gltf");
  if (sponzaModel == nullptr) {
    LOG_WARNING("Sponza model not found at assets/models/Sponza/Sponza.gltf");
    return 1;
  }

  LOG_INFO("Sponza model loaded successfully");

  ecs::TransformComponent sponzaTransform{};
  sponzaTransform.scale = glm::vec3(1.0F);
  resource::SceneLoader::Instantiate(registry, *sponzaModel, sponzaTransform);
  LOG_INFO("Sponza instantiated with {} meshes", sponzaModel->meshes.size());

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
      .farPlane = 1000.0F,  // Increased for Sponza
      .movementSpeed = 5.0F,
      .mouseSensitivity = 0.1F,
  };
  camera::Camera camera{cameraSettings, app.GetWindow().GetAspectRatio()};
  camera.SetPosition(glm::vec3(0.0F, 2.0F, 5.0F));  // Start position
  camera::FPSCameraController cameraController{camera};

  // Handle window resize
  app.GetWindow().AddResizeCallback([&camera, &device](int width, int height) {
    camera.SetAspectRatio(static_cast<float>(width) /
                          static_cast<float>(height));
    device->GetSwapchain()->Resize(width, height);
  });

  // Create render system
  renderer::RenderSystem renderSystem{*device, *factory};

  // Main loop
  app.Run(
      // Update callback
      [&](float deltaTime) {
        // Update camera controller
        cameraController.Update(app.GetInput(), deltaTime);

        // Sync camera data to ECS camera component
        auto& camComp = registry.get<ecs::CameraComponent>(cameraEntity);
        camComp.view = camera.GetView();
        camComp.projection = camera.GetProjection();
        camComp.frustumPlanes = camera.GetFrustumPlanes();
      },
      // Render callback
      [&]() {
        renderSystem.Render(registry, 1.0F / 60.0F);

        // Optional: Print stats every few seconds
        static int frameCount = 0;
        if (++frameCount % 300 == 0) {
          const auto& stats = renderSystem.GetStats();
          LOG_DEBUG("Draw calls: {}, Triangles: {}, Entities: {}",
                    stats.drawCalls, stats.triangles, stats.entitiesProcessed);
        }
      });

  // Wait for GPU to finish before cleanup
  device->WaitIdle();

  return 0;
}
