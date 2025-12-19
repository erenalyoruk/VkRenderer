#include "gpu/context.hpp"

#include <cstdint>
#include <set>
#include <vector>

#include "logger.hpp"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace {
VKAPI_ATTR vk::Bool32 VKAPI_CALL
DebugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
              [[maybe_unused]] vk::DebugUtilsMessageTypeFlagsEXT messageType,
              const vk::DebugUtilsMessengerCallbackDataEXT* callbackData,
              [[maybe_unused]] void* userData) {
  switch (messageSeverity) {
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose:
      LOG_DEBUG("Vulkan: {}", callbackData->pMessage);
      break;
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo:
      LOG_INFO("Vulkan: {}", callbackData->pMessage);
      break;
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning:
      LOG_WARNING("Vulkan: {}", callbackData->pMessage);
      break;
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError:
      LOG_ERROR("Vulkan: {}", callbackData->pMessage);
      break;
    default:
      break;
  }

  return VK_FALSE;
}
}  // namespace

namespace gpu {
Context::Context(Window& window, bool enableValidationLayers)
    : enableValidationLayers_{enableValidationLayers} {
  try {
    InitializeVulkanLoader();

    CreateInstance(window.GetRequiredVulkanExtensions(),
                   enableValidationLayers);
    if (enableValidationLayers) {
      SetupDebugMessenger();
    }

    surface_ = vk::UniqueSurfaceKHR(window.CreateSurface(instance_.get()),
                                    instance_.get());

    SelectPhysicalDevice();
    CreateLogicalDevice();

    LOG_DEBUG("VulkanContext initialized.");
  } catch (const vk::SystemError& err) {
    LOG_CRITICAL("Vulkan initialization failure: {}", err.what());
    throw;
  }
}

Context::~Context() {
  if (device_) {
    device_->waitIdle();
  }
}

void Context::InitializeVulkanLoader() { VULKAN_HPP_DEFAULT_DISPATCHER.init(); }

void Context::CreateInstance(const std::vector<const char*>& windowExtensions,
                             bool enableValidationLayers) {
  vk::ApplicationInfo appInfo{
      .pApplicationName = "Vulkan Particles",
      .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
      .pEngineName = "No Engine",
      .engineVersion = VK_MAKE_VERSION(1, 0, 0),
      .apiVersion = VK_API_VERSION_1_3,
  };

  std::vector<const char*> extensions{VK_EXT_DEBUG_UTILS_EXTENSION_NAME};
  extensions.insert(extensions.end(), windowExtensions.begin(),
                    windowExtensions.end());

  vk::InstanceCreateInfo instanceCreateInfo{
      .pApplicationInfo = &appInfo,
      .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
      .ppEnabledExtensionNames = extensions.data(),
  };

  if (enableValidationLayers) {
    instanceCreateInfo.setPEnabledLayerNames(kValidationLayers);
  }

  instance_ = vk::createInstanceUnique(instanceCreateInfo);

  VULKAN_HPP_DEFAULT_DISPATCHER.init(*instance_);
}

void Context::SetupDebugMessenger() {
  vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo{
      .messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
                         vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
                         vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                         vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
      .messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                     vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                     vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
      .pfnUserCallback = DebugCallback,
  };

  debugMessenger_ =
      instance_->createDebugUtilsMessengerEXTUnique(debugCreateInfo);
}

void Context::SelectPhysicalDevice() {
  auto physicalDevices{instance_->enumeratePhysicalDevices()};
  if (physicalDevices.empty()) {
    LOG_CRITICAL("Failed to find GPUs with Vulkan support!");
    throw std::runtime_error("Failed to find GPUs with Vulkan support!");
  }

  for (const auto& device : physicalDevices) {
    auto props{device.getProperties()};
    if (props.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
      physicalDevice_ = device;
      LOG_INFO("Selected GPU: {}", props.deviceName.data());
      return;
    }
  }

  physicalDevice_ = physicalDevices.front();
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
void Context::CreateLogicalDevice() {
  auto queueFamilies{physicalDevice_.getQueueFamilyProperties()};
  // Find a queue that supports graphics and presentation
  for (uint32_t i = 0; i < queueFamilies.size(); ++i) {
    if (queueFamilies[i].queueFlags & vk::QueueFlagBits::eGraphics) {
      queueFamilyIndices_.graphicsFamily = i;
      if (physicalDevice_.getSurfaceSupportKHR(i, surface_.get()) != 0) {
        queueFamilyIndices_.presentFamily = i;
        break;
      }
    }
  }

  // If we couldn't find a graphics queue that also supports presentation, find
  // them separately
  if (!queueFamilyIndices_.presentFamily.has_value()) {
    for (uint32_t i = 0; i < queueFamilies.size(); ++i) {
      if (physicalDevice_.getSurfaceSupportKHR(i, surface_.get()) != 0) {
        queueFamilyIndices_.presentFamily = i;
        break;
      }
    }
  }

  // Find a dedicated compute queue
  for (uint32_t i = 0; i < queueFamilies.size(); ++i) {
    if ((queueFamilies[i].queueFlags & vk::QueueFlagBits::eCompute) &&
        !(queueFamilies[i].queueFlags & vk::QueueFlagBits::eGraphics)) {
      queueFamilyIndices_.computeFamily = i;
      break;
    }
  }
  // If no dedicated compute queue, find any compute queue
  if (!queueFamilyIndices_.computeFamily.has_value()) {
    for (uint32_t i = 0; i < queueFamilies.size(); ++i) {
      if (queueFamilies[i].queueFlags & vk::QueueFlagBits::eCompute) {
        queueFamilyIndices_.computeFamily = i;
        break;
      }
    }
  }

  // Find a dedicated transfer queue
  for (uint32_t i = 0; i < queueFamilies.size(); ++i) {
    if ((queueFamilies[i].queueFlags & vk::QueueFlagBits::eTransfer) &&
        !(queueFamilies[i].queueFlags & vk::QueueFlagBits::eGraphics) &&
        !(queueFamilies[i].queueFlags & vk::QueueFlagBits::eCompute)) {
      queueFamilyIndices_.transferFamily = i;
      break;
    }
  }
  // If no dedicated transfer queue, find any transfer queue
  if (!queueFamilyIndices_.transferFamily.has_value()) {
    for (uint32_t i = 0; i < queueFamilies.size(); ++i) {
      if (queueFamilies[i].queueFlags & vk::QueueFlagBits::eTransfer) {
        queueFamilyIndices_.transferFamily = i;
        break;
      }
    }
  }

  if (!queueFamilyIndices_.IsComplete()) {
    LOG_CRITICAL("Failed to find all required queue families!");
    throw std::runtime_error("Failed to find all required queue families!");
  }

  float priority{1.0F};
  std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos{};

  std::set<uint32_t> uniqueQueueFamilies = {
      queueFamilyIndices_.graphicsFamily.value(),
      queueFamilyIndices_.computeFamily.value(),
      queueFamilyIndices_.transferFamily.value(),
      queueFamilyIndices_.presentFamily.value(),
  };

  queueCreateInfos.reserve(uniqueQueueFamilies.size());
  for (uint32_t family : uniqueQueueFamilies) {
    vk::DeviceQueueCreateInfo queueCreateInfo{
        .queueFamilyIndex = family,
        .queueCount = 1,
        .pQueuePriorities = &priority,
    };

    queueCreateInfos.push_back(queueCreateInfo);
  }

  vk::PhysicalDeviceVulkan13Features deviceFeatures13{
      .synchronization2 = VK_TRUE,
      .dynamicRendering = VK_TRUE,
  };

  vk::PhysicalDeviceVulkan12Features deviceFeatures12{
      .pNext = &deviceFeatures13,
      .bufferDeviceAddress = VK_TRUE,
  };

  vk::PhysicalDeviceFeatures2 deviceFeatures2{
      .pNext = &deviceFeatures12,
  };

  vk::DeviceCreateInfo createInfo{
      .pNext = &deviceFeatures2,
      .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
      .pQueueCreateInfos = queueCreateInfos.data(),
      .enabledExtensionCount = static_cast<uint32_t>(kDeviceExtensions.size()),
      .ppEnabledExtensionNames = kDeviceExtensions.data(),
      .pEnabledFeatures = nullptr,
  };

  device_ = physicalDevice_.createDeviceUnique(createInfo);

  VULKAN_HPP_DEFAULT_DISPATCHER.init(*device_);

  graphicsQueue_ =
      device_->getQueue(queueFamilyIndices_.graphicsFamily.value(), 0);

  computeQueue_ =
      device_->getQueue(queueFamilyIndices_.computeFamily.value(), 0);

  transferQueue_ =
      device_->getQueue(queueFamilyIndices_.transferFamily.value(), 0);
}
}  // namespace gpu
