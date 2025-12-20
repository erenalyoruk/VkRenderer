#pragma once

#include <memory>
#include <vector>

#include <vulkan/vulkan.hpp>

#include "rhi/shader.hpp"

namespace backends::vulkan {
class VulkanContext;

class VulkanShader : public rhi::Shader {
 public:
  static std::unique_ptr<VulkanShader> Create(VulkanContext& context,
                                              rhi::ShaderStage stage,
                                              std::span<const uint32_t> spirv);

  // RHI implementations
  [[nodiscard]] rhi::ShaderStage GetStage() const override { return stage_; }
  [[nodiscard]] const std::vector<uint32_t>& GetSPIRVCode() const override {
    return spirv_;
  }

  // Vulkan getter
  [[nodiscard]] vk::ShaderModule GetShaderModule() const {
    return shaderModule_.get();
  }

 private:
  VulkanShader(rhi::ShaderStage stage, std::vector<uint32_t> spirv,
               vk::UniqueShaderModule shaderModule);

  rhi::ShaderStage stage_;
  std::vector<uint32_t> spirv_;
  vk::UniqueShaderModule shaderModule_;
};
}  // namespace backends::vulkan
