#include "backends/vulkan/vulkan_shader.hpp"

#include <utility>

#include "backends/vulkan/vulkan_context.hpp"

namespace backends::vulkan {
std::unique_ptr<VulkanShader> VulkanShader::Create(
    VulkanContext& context, rhi::ShaderStage stage,
    std::span<const uint32_t> spirv) {
  vk::ShaderModuleCreateInfo createInfo{
      .codeSize = spirv.size() * sizeof(uint32_t),
      .pCode = spirv.data(),
  };

  vk::UniqueShaderModule shaderModule{
      context.GetDevice().createShaderModuleUnique(createInfo)};

  std::vector<uint32_t> spirvCopy{spirv.begin(), spirv.end()};

  return std::unique_ptr<VulkanShader>(
      new VulkanShader(stage, std::move(spirvCopy), std::move(shaderModule)));
}

VulkanShader::VulkanShader(rhi::ShaderStage stage, std::vector<uint32_t> spirv,
                           vk::UniqueShaderModule shaderModule)
    : stage_{stage},
      spirv_{std::move(spirv)},
      shaderModule_{std::move(shaderModule)} {}
}  // namespace backends::vulkan
