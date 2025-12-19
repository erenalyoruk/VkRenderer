#include "application.hpp"
#include "gpu/renderer.hpp"
#include "logger.hpp"

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
  gpu::Renderer renderer{app.GetWindow()};

  app.GetWindow().SetOnResize(
      [&renderer](int width, int height) { renderer.Resize(width, height); });

  app.Run([&]() { renderer.RenderFrame(); });

  return 0;
}
