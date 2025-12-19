#pragma once

#include <functional>

#include <vulkan/vulkan.hpp>

#include "gpu/context.hpp"

namespace gpu {
class CommandSystem {
 public:
  explicit CommandSystem(Context& context);
  ~CommandSystem() = default;

  CommandSystem(const CommandSystem&) = delete;
  CommandSystem& operator=(const CommandSystem&) = delete;

  CommandSystem(CommandSystem&&) = delete;
  CommandSystem& operator=(CommandSystem&&) = delete;

  [[nodiscard]] vk::UniqueCommandBuffer CreateGraphicsCommandBuffer() const;
  [[nodiscard]] vk::UniqueCommandBuffer CreateTransferCommandBuffer() const;
  [[nodiscard]] vk::UniqueCommandBuffer CreateComputeCommandBuffer() const;

  void ImmediateGraphicsSubmit(
      const std::function<void(vk::CommandBuffer)>& fn);
  void ImmediateTransferSubmit(
      const std::function<void(vk::CommandBuffer)>& fn);

  void ImmediateComputeSubmit(const std::function<void(vk::CommandBuffer)>& fn);

 private:
  Context& context_;

  vk::UniqueCommandPool graphicsCommandPool_;
  vk::UniqueCommandPool transferCommandPool_;
  vk::UniqueCommandPool computeCommandPool_;
};
}  // namespace gpu
