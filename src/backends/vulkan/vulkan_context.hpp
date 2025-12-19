#pragma once

#include <memory>
#include <optional>
#include <vector>

#include <vulkan/vulkan.hpp>

#include "backends/vulkan/vulkan_allocator.hpp"
#include "rhi/device.hpp"
#include "rhi/queue.hpp"
#include "window.hpp"

namespace backends::vulkan {
class VulkanQueue : public rhi::Queue {
 public:
  VulkanQueue(vk::Queue queue, rhi::QueueType type)
      : queue_(queue), type_(type) {}

  void Submit(/* params */) override { /* Implement */ }
  void Present(/* params */) override { /* Implement */ }

  [[nodiscard]] rhi::QueueType GetType() const override { return type_; }

 private:
  vk::Queue queue_;
  rhi::QueueType type_;
};

class VulkanContext : public rhi::Device {
 public:
  VulkanContext(Window& window, bool enableValidationLayers = false);
  ~VulkanContext() override;

  // RHI interface implementations
  [[nodiscard]] rhi::Queue* GetQueue(rhi::QueueType type) override;

  [[nodiscard]] rhi::Swapchain* GetSwapchain() override {
    return nullptr;  // TODO: Implement swapchain
  }

  // Vulkan-specific getters (for internal usage)
  [[nodiscard]] vk::Instance GetInstance() const { return instance_.get(); }

  [[nodiscard]] vk::PhysicalDevice GetPhysicalDevice() const {
    return physicalDevice_;
  }

  [[nodiscard]] vk::Device GetDevice() const { return device_.get(); }

  [[nodiscard]] vk::SurfaceKHR GetSurface() const { return surface_.get(); }

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

  QueueFamilyIndices queueFamilyIndices_;
  vk::Queue graphicsQueue_{VK_NULL_HANDLE};
  vk::Queue computeQueue_{VK_NULL_HANDLE};
  vk::Queue transferQueue_{VK_NULL_HANDLE};

  std::unique_ptr<VulkanAllocator> allocator_;
  std::vector<std::unique_ptr<VulkanQueue>> queues_;

  void CreateInstance(const std::vector<const char*>& windowExtensions,
                      bool enableValidationLayers);
  void SetupDebugMessenger();
  void SelectPhysicalDevice();
  void CreateLogicalDevice();
};
}  // namespace backends::vulkan
