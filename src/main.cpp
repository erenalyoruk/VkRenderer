#include "application.hpp"
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

  auto [device, factory]{rhi::BackendFactory::Create(rhi::BackendType::Vulkan,
                                                     app.GetWindow(), true)};

  return 0;
}
