#include "gpu/instance.hpp"

#include <set>

#include "logger.hpp"

namespace {
bool CheckValidationLayerSupport(const std::vector<const char*>& layers) {
  auto availableLayers{vk::enumerateInstanceLayerProperties()};
  for (const char* layerName : layers) {
    bool found{false};
    for (const auto& layerProperties : availableLayers) {
      if (std::string(layerName) == layerProperties.layerName) {
        found = true;
        break;
      }
    }

    if (!found) {
      return false;
    }
  }

  return true;
}

VKAPI_ATTR VkBool32 VKAPI_CALL DebugUtilsMessengerCallback(
    vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    vk::DebugUtilsMessageTypeFlagsEXT /*messageType*/,
    const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* /*pUserData*/) {
  switch (messageSeverity) {
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose:
      LOG_DEBUG("VULKAN VALIDATION LAYER: {}", pCallbackData->pMessage);
      break;
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo:
      LOG_INFO("VULKAN VALIDATION LAYER: {}", pCallbackData->pMessage);
      break;
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning:
      LOG_WARNING("VULKAN VALIDATION LAYER: {}", pCallbackData->pMessage);
      break;
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError:
      LOG_ERROR("VULKAN VALIDATION LAYER: {}", pCallbackData->pMessage);
      break;
    default:
      LOG_INFO("VULKAN VALIDATION LAYER: {}", pCallbackData->pMessage);
      break;
  }

  return VK_FALSE;
}

vk::UniqueDebugUtilsMessengerEXT CreateDebugUtilsMessenger(
    vk::Instance& instance) {
  vk::DebugUtilsMessengerCreateInfoEXT createInfo{
      .flags = {},
      .messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
                         vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                         vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
      .messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                     vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                     vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
      .pfnUserCallback = DebugUtilsMessengerCallback,
      .pUserData = nullptr,
  };

  return instance.createDebugUtilsMessengerEXTUnique(createInfo);
}
}  // namespace

namespace gpu {
Instance::Instance(const std::string& appName,
                   std::vector<const char*> requiredExtensions) {
  VULKAN_HPP_DEFAULT_DISPATCHER.init();

  std::set<const char*> uniqueExtensions{kExtensions.begin(),
                                         kExtensions.end()};
  uniqueExtensions.insert(requiredExtensions.begin(), requiredExtensions.end());
  std::vector<const char*> finalExtensions{uniqueExtensions.begin(),
                                           uniqueExtensions.end()};

#ifdef NDEBUG
  std::vector<const char*> validationLayers{};
#else
  std::vector<const char*> validationLayers{kValidationLayers.begin(),
                                            kValidationLayers.end()};
  if (!CheckValidationLayerSupport(validationLayers)) {
    LOG_CRITICAL("Validation layers requested, but not available!");
    throw std::runtime_error("Validation layers requested, but not available!");
  }
#endif

  vk::ApplicationInfo appInfo{
      .pApplicationName = appName.c_str(),
      .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
      .pEngineName = "No Engine",
      .engineVersion = VK_MAKE_VERSION(1, 0, 0),
      .apiVersion = VK_API_VERSION_1_4,
  };

  vk::InstanceCreateInfo createInfo{
      .pApplicationInfo = &appInfo,
      .enabledLayerCount = static_cast<uint32_t>(validationLayers.size()),
      .ppEnabledLayerNames = validationLayers.data(),
      .enabledExtensionCount = static_cast<uint32_t>(finalExtensions.size()),
      .ppEnabledExtensionNames = finalExtensions.data(),
  };

  try {
    instance_ = vk::createInstanceUnique(createInfo);
  } catch (const vk::SystemError& err) {
    LOG_CRITICAL("Failed to create Vulkan instance: {}", err.what());
    throw;
  }

  VULKAN_HPP_DEFAULT_DISPATCHER.init(*instance_);

#ifdef NDEBUG
  debugMessenger_ = nullptr;
#else
  debugMessenger_ = CreateDebugUtilsMessenger(*instance_);
#endif

  LOG_INFO("Vulkan instance created successfully.");
}

Instance::~Instance() { LOG_INFO("Destroying Vulkan instance."); }
}  // namespace gpu
