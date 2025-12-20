#pragma once

#include <bit>
#include <memory>
#include <optional>
#include <vector>

#include <vulkan/vulkan.hpp>

#include "backends/vulkan/vulkan_allocator.hpp"
#include "rhi/command.hpp"
#include "rhi/device.hpp"
#include "rhi/queue.hpp"
#include "rhi/sync.hpp"
#include "window.hpp"

namespace backends::vulkan {
class VulkanSwapchain;

class VulkanQueue : public rhi::Queue {
 public:
  VulkanQueue(vk::Queue queue, rhi::QueueType type);

  void Submit(std::span<rhi::CommandBuffer* const> commandBuffers,
              std::span<rhi::Semaphore* const> waitSemaphores,
              std::span<rhi::Semaphore* const> signalSemaphores,
              rhi::Fence* fence) override;

  void Present(rhi::Swapchain* swapchain, uint32_t imageIndex,
               std::span<rhi::Semaphore* const> waitSemaphores) override;

  [[nodiscard]] rhi::QueueType GetType() const override { return type_; }

 private:
  vk::Queue queue_;
  rhi::QueueType type_;
};

class VulkanContext : public rhi::Device {
 public:
  VulkanContext(class Window& window, uint32_t width, uint32_t height,
                bool enableValidationLayers = false);
  ~VulkanContext() override;

  // RHI interface implementations
  void WaitIdle() override { device_->waitIdle(); }

  [[nodiscard]] rhi::Queue* GetQueue(rhi::QueueType type) override;

  [[nodiscard]] rhi::Swapchain* GetSwapchain() override {
    return std::bit_cast<rhi::Swapchain*>(swapchain_.get());
  }

  // Vulkan-specific getters (for internal usage)
  [[nodiscard]] vk::Instance GetInstance() const { return instance_.get(); }

  [[nodiscard]] vk::PhysicalDevice GetPhysicalDevice() const {
    return physicalDevice_;
  }

  [[nodiscard]] vk::Device GetDevice() const { return device_.get(); }

  [[nodiscard]] vk::SurfaceKHR GetSurface() const { return surface_.get(); }

  [[nodiscard]] vk::DescriptorPool GetDescriptorPool() const {
    return descriptorPool_.get();
  }

  [[nodiscard]] uint32_t GetGraphicsFamilyIndex() const {
    return queueFamilyIndices_.graphicsFamily.value();
  }
  [[nodiscard]] uint32_t GetComputeFamilyIndex() const {
    return queueFamilyIndices_.computeFamily.value();
  }
  [[nodiscard]] uint32_t GetTransferFamilyIndex() const {
    return queueFamilyIndices_.transferFamily.value_or(
        queueFamilyIndices_.graphicsFamily.value());
  }

  VulkanAllocator& GetAllocator() { return *allocator_; }

 private:
  struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> computeFamily;
    std::optional<uint32_t> transferFamily;
    std::optional<uint32_t> presentFamily;

    [[nodiscard]] bool IsComplete() const {
      return graphicsFamily.has_value() && computeFamily.has_value() &&
             transferFamily.has_value() && presentFamily.has_value();
    }
  };

  bool enableValidation_{false};

  vk::UniqueInstance instance_;
  vk::UniqueDebugUtilsMessengerEXT debugMessenger_;
  vk::UniqueSurfaceKHR surface_;
  vk::PhysicalDevice physicalDevice_;
  vk::UniqueDevice device_;
  vk::UniqueDescriptorPool descriptorPool_;

  QueueFamilyIndices queueFamilyIndices_;
  vk::Queue graphicsQueue_{VK_NULL_HANDLE};
  vk::Queue computeQueue_{VK_NULL_HANDLE};
  vk::Queue transferQueue_{VK_NULL_HANDLE};

  std::unique_ptr<VulkanAllocator> allocator_;
  std::vector<std::unique_ptr<VulkanQueue>> queues_;
  std::unique_ptr<VulkanSwapchain> swapchain_;

  void CreateInstance(const std::vector<const char*>& windowExtensions,
                      bool enableValidationLayers);
  void SetupDebugMessenger();
  void SelectPhysicalDevice();
  void CreateLogicalDevice();
};
}  // namespace backends::vulkan
