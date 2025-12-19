#include "application.hpp"
#include "logger.hpp"
#include "rendering/renderer.hpp"

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
  rendering::Renderer renderer{app.GetWindow(), true};

  app.Run([&]() {
    renderer.SetViewProjection(app.GetCamera().GetViewProjectionMatrix());
    renderer.RenderFrame();
  });

  return 0;
}
