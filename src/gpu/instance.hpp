#pragma once

#include <array>
#include <string>
#include <vector>

#include <vulkan/vulkan.hpp>

namespace gpu {
class Instance {
 public:
  Instance(const std::string& appName,
           std::vector<const char*> requiredExtensions);
  ~Instance();

  Instance(const Instance&) = delete;
  Instance& operator=(const Instance&) = delete;

  Instance(Instance&&) = default;
  Instance& operator=(Instance&&) = default;

  [[nodiscard]] vk::Instance GetHandle() const { return *instance_; }

 private:
  vk::UniqueInstance instance_{VK_NULL_HANDLE};
  vk::UniqueDebugUtilsMessengerEXT debugMessenger_{VK_NULL_HANDLE};

  constexpr static std::array<const char*, 1> kValidationLayers{
      "VK_LAYER_KHRONOS_validation",
  };

  constexpr static std::array<const char*, 2> kExtensions{
      VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
      VK_KHR_SURFACE_EXTENSION_NAME,
  };
};
}  // namespace gpu
