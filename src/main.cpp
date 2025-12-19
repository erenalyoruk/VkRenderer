#include "application.hpp"
#include "logger.hpp"
#include "vulkan/renderer.hpp"

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
  vulkan::Renderer renderer{app.GetWindow()};

  app.Run([&]() { renderer.RenderFrame(); });

  return 0;
}
