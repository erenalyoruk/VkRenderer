#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

#include <vulkan/vulkan.hpp>

#include "vulkan/allocator.hpp"
#include "window.hpp"

namespace vulkan {
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

class Context {
 public:
  explicit Context(Window& window, bool enableValidationLayers = true);
  ~Context();

  Context(const Context&) = delete;
  Context& operator=(const Context&) = delete;

  Context(Context&&) = delete;
  Context& operator=(Context&&) = delete;

  [[nodiscard]] vk::Instance GetInstance() const { return instance_.get(); }
  [[nodiscard]] vk::PhysicalDevice GetPhysicalDevice() const {
    return physicalDevice_;
  }
  [[nodiscard]] vk::Device GetDevice() const { return device_.get(); }
  [[nodiscard]] vk::SurfaceKHR GetSurface() const { return surface_.get(); }

  [[nodiscard]] vk::Queue GetGraphicsQueue() const { return graphicsQueue_; }
  [[nodiscard]] vk::Queue GetTransferQueue() const { return transferQueue_; }
  [[nodiscard]] vk::Queue GetComputeQueue() const { return computeQueue_; }
  [[nodiscard]] const QueueFamilyIndices& GetQueueFamilyIndices() const {
    return queueFamilyIndices_;
  }

  [[nodiscard]] Allocator& GetAllocator() const { return *allocator_; }

 private:
  const std::vector<const char*> kValidationLayers{
      "VK_LAYER_KHRONOS_validation"};

  const std::vector<const char*> kDeviceExtensions{
      VK_KHR_SWAPCHAIN_EXTENSION_NAME,
      "VK_KHR_buffer_device_address",
  };

  bool enableValidationLayers_{false};

  vk::UniqueInstance instance_{VK_NULL_HANDLE};
  vk::UniqueDebugUtilsMessengerEXT debugMessenger_{VK_NULL_HANDLE};
  vk::UniqueSurfaceKHR surface_{VK_NULL_HANDLE};

  vk::PhysicalDevice physicalDevice_{VK_NULL_HANDLE};
  vk::UniqueDevice device_{VK_NULL_HANDLE};

  QueueFamilyIndices queueFamilyIndices_{};
  vk::Queue graphicsQueue_{VK_NULL_HANDLE};
  vk::Queue computeQueue_{VK_NULL_HANDLE};
  vk::Queue transferQueue_{VK_NULL_HANDLE};

  std::unique_ptr<Allocator> allocator_{nullptr};

  static void InitializeVulkanLoader();
  void CreateInstance(const std::vector<const char*>& windowExtensions,
                      bool enableValidationLayers);
  void SelectPhysicalDevice();
  void CreateLogicalDevice();
  void SetupDebugMessenger();
};
}  // namespace vulkan
