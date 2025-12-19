#include "gpu/command_system.hpp"

#include <cstdint>
#include <utility>

#include "logger.hpp"

namespace gpu {
CommandSystem::CommandSystem(Context& context) : context_(context) {
  vk::CommandPoolCreateInfo graphicsPoolInfo{
      .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
      .queueFamilyIndex =
          context.GetQueueFamilyIndices().graphicsFamily.value(),
  };

  graphicsCommandPool_ =
      context.GetDevice().createCommandPoolUnique(graphicsPoolInfo);

  vk::CommandPoolCreateInfo computePoolInfo{
      .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
      .queueFamilyIndex = context.GetQueueFamilyIndices().computeFamily.value(),
  };
  computeCommandPool_ =
      context.GetDevice().createCommandPoolUnique(computePoolInfo);

  vk::CommandPoolCreateInfo transferPoolInfo{
      .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
      .queueFamilyIndex =
          context.GetQueueFamilyIndices().transferFamily.value(),
  };
  transferCommandPool_ =
      context.GetDevice().createCommandPoolUnique(transferPoolInfo);

  LOG_DEBUG(
      "CommandSystem initialized (Graphics & Compute & Transfer pools ready)");
}

vk::UniqueCommandBuffer CommandSystem::CreateGraphicsCommandBuffer() const {
  vk::CommandBufferAllocateInfo allocInfo{
      .commandPool = graphicsCommandPool_.get(),
      .level = vk::CommandBufferLevel::ePrimary,
      .commandBufferCount = 1,
  };

  return std::move(
      context_.GetDevice().allocateCommandBuffersUnique(allocInfo).front());
}

vk::UniqueCommandBuffer CommandSystem::CreateComputeCommandBuffer() const {
  vk::CommandBufferAllocateInfo allocInfo{
      .commandPool = computeCommandPool_.get(),
      .level = vk::CommandBufferLevel::ePrimary,
      .commandBufferCount = 1,
  };

  return std::move(
      context_.GetDevice().allocateCommandBuffersUnique(allocInfo).front());
}

vk::UniqueCommandBuffer CommandSystem::CreateTransferCommandBuffer() const {
  vk::CommandBufferAllocateInfo allocInfo{
      .commandPool = transferCommandPool_.get(),
      .level = vk::CommandBufferLevel::ePrimary,
      .commandBufferCount = 1,
  };

  return std::move(
      context_.GetDevice().allocateCommandBuffersUnique(allocInfo).front());
}

void CommandSystem::ImmediateGraphicsSubmit(
    const std::function<void(vk::CommandBuffer)>& fn) {
  vk::Device device{context_.GetDevice()};
  vk::Queue graphicsQueue{context_.GetGraphicsQueue()};

  vk::UniqueCommandBuffer commandBuffer{CreateGraphicsCommandBuffer()};
  commandBuffer->begin(vk::CommandBufferBeginInfo{
      .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
  fn(commandBuffer.get());
  commandBuffer->end();
  vk::SubmitInfo submitInfo{
      .commandBufferCount = 1,
      .pCommandBuffers = &commandBuffer.get(),
  };

  vk::UniqueFence fence{device.createFenceUnique(vk::FenceCreateInfo{})};

  graphicsQueue.submit(submitInfo, fence.get());

  vk::Result result{device.waitForFences(fence.get(), VK_TRUE, UINT64_MAX)};
  if (result != vk::Result::eSuccess) {
    LOG_ERROR("Failed to wait for graphics immediate submit fence");
  }
}

void CommandSystem::ImmediateComputeSubmit(
    const std::function<void(vk::CommandBuffer)>& fn) {
  vk::Device device{context_.GetDevice()};
  vk::Queue computeQueue{context_.GetComputeQueue()};

  vk::UniqueCommandBuffer commandBuffer{CreateComputeCommandBuffer()};
  commandBuffer->begin(vk::CommandBufferBeginInfo{
      .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
  fn(commandBuffer.get());
  commandBuffer->end();
  vk::SubmitInfo submitInfo{
      .commandBufferCount = 1,
      .pCommandBuffers = &commandBuffer.get(),
  };

  vk::UniqueFence fence{device.createFenceUnique(vk::FenceCreateInfo{})};

  computeQueue.submit(submitInfo, fence.get());

  vk::Result result{device.waitForFences(fence.get(), VK_TRUE, UINT64_MAX)};
  if (result != vk::Result::eSuccess) {
    LOG_ERROR("Failed to wait for compute immediate submit fence");
  }
}

void CommandSystem::ImmediateTransferSubmit(
    const std::function<void(vk::CommandBuffer)>& fn) {
  vk::Device device{context_.GetDevice()};
  vk::Queue transferQueue{context_.GetTransferQueue()};

  vk::UniqueCommandBuffer commandBuffer{CreateTransferCommandBuffer()};
  commandBuffer->begin(vk::CommandBufferBeginInfo{
      .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
  fn(commandBuffer.get());
  commandBuffer->end();
  vk::SubmitInfo submitInfo{
      .commandBufferCount = 1,
      .pCommandBuffers = &commandBuffer.get(),
  };

  vk::UniqueFence fence{device.createFenceUnique(vk::FenceCreateInfo{})};

  transferQueue.submit(submitInfo, fence.get());

  vk::Result result{device.waitForFences(fence.get(), VK_TRUE, UINT64_MAX)};
  if (result != vk::Result::eSuccess) {
    LOG_ERROR("Failed to wait for transfer immediate submit fence");
  }
}
}  // namespace gpu
