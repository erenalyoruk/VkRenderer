#include "application.hpp"
#include "camera/camera_controller.hpp"
#include "camera/fps_camera_controller.hpp"
#include "logger.hpp"
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

  Application app(1280, 720, "Vulkan Renderer");

  auto [device, factory]{rhi::BackendFactory::Create(
      rhi::BackendType::Vulkan, app.GetWindow(), app.GetWindow().GetWidth(),
      app.GetWindow().GetHeight(), true)};

  camera::CameraSettings cameraSettings{
      .fov = glm::radians(60.0F),
      .nearPlane = 0.1F,
      .farPlane = 100.0F,
      .movementSpeed = 5.0F,
      .mouseSensitivity = 0.1F,
  };
  camera::Camera camera{cameraSettings, app.GetWindow().GetAspectRatio()};
  camera::FPSCameraController cameraController{camera};

  app.GetWindow().AddResizeCallback([&camera](int width, int height) {
    camera.SetAspectRatio(static_cast<float>(width) /
                          static_cast<float>(height));
  });

  app.Run(
      [&](float deltaTime) {
        cameraController.Update(app.GetInput(), deltaTime);
      },
      [&]() {});

  return 0;
}
