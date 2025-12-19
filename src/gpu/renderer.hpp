#pragma once

#include <memory>

#include "gpu/command_system.hpp"
#include "gpu/context.hpp"
#include "gpu/frame_manager.hpp"
#include "gpu/swapchain.hpp"
#include "window.hpp"

namespace gpu {
class Renderer {
 public:
  explicit Renderer(Window& window, bool enableValidationLayers = true);
  ~Renderer();

  void RenderFrame();

  void Resize(int width, int height);

  [[nodiscard]] bool IsInitialized() const { return isInitialized_; }

 private:
  Window& window_;
  bool enableValidationLayers_;

  std::unique_ptr<Context> context_;
  std::unique_ptr<CommandSystem> commandSystem_;
  std::unique_ptr<Swapchain> swapchain_;
  std::unique_ptr<FrameManager> frameManager_;

  bool isInitialized_{false};
};
}  // namespace gpu
