#include "backends/vulkan/vulkan_context.hpp"

#include <array>
#include <bit>
#include <cstdint>
#include <set>
#include <vector>

#include "backends/vulkan/vulkan_command.hpp"
#include "backends/vulkan/vulkan_fence.hpp"
#include "backends/vulkan/vulkan_semaphore.hpp"
#include "backends/vulkan/vulkan_swapchain.hpp"
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

namespace backends::vulkan {
VulkanQueue::VulkanQueue(vk::Queue queue, rhi::QueueType type)
    : queue_{queue}, type_{type} {}

void VulkanQueue::Submit(std::span<rhi::CommandBuffer* const> commandBuffers,
                         std::span<rhi::Semaphore* const> waitSemaphores,
                         std::span<rhi::Semaphore* const> signalSemaphores,
                         rhi::Fence* fence) {
  std::vector<vk::CommandBuffer> vkCmdBuffers{};
  for (auto* cb : commandBuffers) {
    auto* vkCb{std::bit_cast<VulkanCommandBuffer*>(cb)};
    vkCmdBuffers.push_back(vkCb->GetCommandBuffer());
  }

  std::vector<vk::Semaphore> vkWaitSemaphores{};
  std::vector<vk::PipelineStageFlags> waitStages{};
  for (auto* sem : waitSemaphores) {
    auto* vkSem{std::bit_cast<VulkanSemaphore*>(sem)};
    vkWaitSemaphores.push_back(vkSem->GetSemaphore());
    waitStages.emplace_back(vk::PipelineStageFlagBits::eColorAttachmentOutput);
  }

  std::vector<vk::Semaphore> vkSignalSemaphores{};
  for (auto* sem : signalSemaphores) {
    auto* vkSem{std::bit_cast<VulkanSemaphore*>(sem)};
    vkSignalSemaphores.push_back(vkSem->GetSemaphore());
  }

  vk::SubmitInfo submitInfo{
      .waitSemaphoreCount = static_cast<uint32_t>(vkWaitSemaphores.size()),
      .pWaitSemaphores = vkWaitSemaphores.data(),
      .pWaitDstStageMask = waitStages.data(),
      .commandBufferCount = static_cast<uint32_t>(vkCmdBuffers.size()),
      .pCommandBuffers = vkCmdBuffers.data(),
      .signalSemaphoreCount = static_cast<uint32_t>(vkSignalSemaphores.size()),
      .pSignalSemaphores = vkSignalSemaphores.data(),
  };

  vk::Fence vkFence{(fence != nullptr)
                        ? std::bit_cast<VulkanFence*>(fence)->GetFence()
                        : VK_NULL_HANDLE};
  queue_.submit(submitInfo, vkFence);
}

void VulkanQueue::Present(rhi::Swapchain* swapchain, uint32_t imageIndex,
                          std::span<rhi::Semaphore* const> waitSemaphores) {
  std::vector<vk::Semaphore> vkWaitSemaphores;
  for (auto* sem : waitSemaphores) {
    vkWaitSemaphores.push_back(
        std::bit_cast<VulkanSemaphore*>(sem)->GetSemaphore());
  }

  vk::SwapchainKHR vkSwapchain =
      std::bit_cast<VulkanSwapchain*>(swapchain)->GetSwapchain();
  vk::PresentInfoKHR presentInfo{
      .waitSemaphoreCount = static_cast<uint32_t>(vkWaitSemaphores.size()),
      .pWaitSemaphores = vkWaitSemaphores.data(),
      .swapchainCount = 1,
      .pSwapchains = &vkSwapchain,
      .pImageIndices = &imageIndex,
  };

  auto result{queue_.presentKHR(presentInfo)};
  if (result != vk::Result::eSuccess) {
    LOG_ERROR("Failed to present swapchain image: {}", vk::to_string(result));
  }
}

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

VulkanContext::VulkanContext(class Window& window, uint32_t width,
                             uint32_t height, bool enableValidationLayers)
    : enableValidation_{enableValidationLayers} {
  try {
    VULKAN_HPP_DEFAULT_DISPATCHER.init();

    auto extensions = window.GetRequiredVulkanExtensions();
    CreateInstance(extensions, enableValidationLayers);
    if (enableValidationLayers) {
      SetupDebugMessenger();
    }

    surface_ = vk::UniqueSurfaceKHR(window.CreateSurface(instance_.get()),
                                    instance_.get());

    SelectPhysicalDevice();
    CreateLogicalDevice();

    allocator_ = std::make_unique<VulkanAllocator>(*this);
    swapchain_ = VulkanSwapchain::Create(*this, width, height,
                                         rhi::Format::R8G8B8A8Unorm);

    std::array<vk::DescriptorPoolSize, 5> poolSizes{{
        {.type = vk::DescriptorType::eUniformBuffer, .descriptorCount = 1000},
        {.type = vk::DescriptorType::eStorageBuffer, .descriptorCount = 100},
        {.type = vk::DescriptorType::eSampledImage, .descriptorCount = 1000},
        {.type = vk::DescriptorType::eSampler, .descriptorCount = 100},
        {.type = vk::DescriptorType::eCombinedImageSampler,
         .descriptorCount = 1000},
    }};

    vk::DescriptorPoolCreateInfo poolInfo{
        .maxSets = 1000,
        .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
        .pPoolSizes = poolSizes.data(),
    };

    descriptorPool_ = device_->createDescriptorPoolUnique(poolInfo);

    // Create RHI queues
    queues_.push_back(std::make_unique<VulkanQueue>(graphicsQueue_,
                                                    rhi::QueueType::Graphics));
    queues_.push_back(
        std::make_unique<VulkanQueue>(computeQueue_, rhi::QueueType::Compute));
    queues_.push_back(std::make_unique<VulkanQueue>(transferQueue_,
                                                    rhi::QueueType::Transfer));

    LOG_DEBUG("VulkanContext initialized.");
  } catch (const vk::SystemError& err) {
    LOG_CRITICAL("Vulkan initialization failure: {}", err.what());
    throw;
  }
}

VulkanContext::~VulkanContext() {
  if (device_) {
    device_->waitIdle();
  }
  allocator_.reset();
}

rhi::Queue* VulkanContext::GetQueue(rhi::QueueType type) {
  for (auto& q : queues_) {
    if (q->GetType() == type) {
      return q.get();
    }
  }

  return nullptr;
}

void VulkanContext::CreateInstance(
    const std::vector<const char*>& windowExtensions,
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

  const std::vector<const char*> kValidationLayers{
      "VK_LAYER_KHRONOS_validation"};
  if (enableValidationLayers) {
    instanceCreateInfo.setPEnabledLayerNames(kValidationLayers);
  }

  instance_ = vk::createInstanceUnique(instanceCreateInfo);
  VULKAN_HPP_DEFAULT_DISPATCHER.init(*instance_);
}

void VulkanContext::SetupDebugMessenger() {
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

void VulkanContext::SelectPhysicalDevice() {
  auto physicalDevices = instance_->enumeratePhysicalDevices();
  if (physicalDevices.empty()) {
    LOG_CRITICAL("Failed to find GPUs with Vulkan support!");
    throw std::runtime_error("Failed to find GPUs with Vulkan support!");
  }

  for (const auto& device : physicalDevices) {
    auto props = device.getProperties();
    if (props.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
      physicalDevice_ = device;
      LOG_INFO("Selected GPU: {}", props.deviceName.data());
      return;
    }
  }

  physicalDevice_ = physicalDevices.front();
}

void VulkanContext::CreateLogicalDevice() {
  auto queueFamilies = physicalDevice_.getQueueFamilyProperties();
  QueueFamilyIndices& indices = queueFamilyIndices_;

  // Find graphics and present queue
  for (uint32_t i = 0; i < queueFamilies.size(); ++i) {
    if (queueFamilies[i].queueFlags & vk::QueueFlagBits::eGraphics) {
      indices.graphicsFamily = i;
      if (physicalDevice_.getSurfaceSupportKHR(i, surface_.get()) == VK_TRUE) {
        indices.presentFamily = i;
      }
      break;
    }
  }

  if (!indices.presentFamily.has_value()) {
    for (uint32_t i = 0; i < queueFamilies.size(); ++i) {
      if (physicalDevice_.getSurfaceSupportKHR(i, surface_.get()) == VK_TRUE) {
        indices.presentFamily = i;
        break;
      }
    }
  }

  // Find compute queue
  for (uint32_t i = 0; i < queueFamilies.size(); ++i) {
    if ((queueFamilies[i].queueFlags & vk::QueueFlagBits::eCompute) &&
        !(queueFamilies[i].queueFlags & vk::QueueFlagBits::eGraphics)) {
      indices.computeFamily = i;
      break;
    }
  }
  if (!indices.computeFamily.has_value()) {
    for (uint32_t i = 0; i < queueFamilies.size(); ++i) {
      if (queueFamilies[i].queueFlags & vk::QueueFlagBits::eCompute) {
        indices.computeFamily = i;
        break;
      }
    }
  }

  // Find transfer queue
  for (uint32_t i = 0; i < queueFamilies.size(); ++i) {
    if ((queueFamilies[i].queueFlags & vk::QueueFlagBits::eTransfer) &&
        !(queueFamilies[i].queueFlags & vk::QueueFlagBits::eGraphics) &&
        !(queueFamilies[i].queueFlags & vk::QueueFlagBits::eCompute)) {
      indices.transferFamily = i;
      break;
    }
  }
  if (!indices.transferFamily.has_value()) {
    for (uint32_t i = 0; i < queueFamilies.size(); ++i) {
      if (queueFamilies[i].queueFlags & vk::QueueFlagBits::eTransfer) {
        indices.transferFamily = i;
        break;
      }
    }
  }

  if (!indices.IsComplete()) {
    LOG_CRITICAL("Failed to find all required queue families!");
    throw std::runtime_error("Failed to find all required queue families!");
  }

  float priority = 1.0F;
  std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
  std::set<uint32_t> uniqueQueueFamilies = {
      indices.graphicsFamily.value(),
      indices.computeFamily.value(),
      indices.transferFamily.value(),
      indices.presentFamily.value(),
  };

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
      .features =
          vk::PhysicalDeviceFeatures{
              .fillModeNonSolid = VK_TRUE,
          },
  };

  const std::vector<const char*> kDeviceExtensions{
      VK_KHR_SWAPCHAIN_EXTENSION_NAME,
      "VK_KHR_buffer_device_address",
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

  graphicsQueue_ = device_->getQueue(indices.graphicsFamily.value(), 0);
  computeQueue_ = device_->getQueue(indices.computeFamily.value(), 0);
  transferQueue_ = device_->getQueue(indices.transferFamily.value(), 0);
}
}  // namespace backends::vulkan
