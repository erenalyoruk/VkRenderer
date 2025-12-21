#pragma once

#include <memory>
#include <optional>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vulkan/vulkan.hpp>

#include "rhi/sampler.hpp"

namespace backends::vulkan {
class VulkanContext;

class VulkanSampler : public rhi::Sampler {
 public:
  static std::unique_ptr<VulkanSampler> Create(
      VulkanContext& context, rhi::Filter magFilter, rhi::Filter minFilter,
      rhi::AddressMode addressModeU, rhi::AddressMode addressModeV,
      std::optional<std::span<const float, 4>> borderColor = std::nullopt,
      bool compareEnable = false,
      rhi::CompareOp compareOp = rhi::CompareOp::Always);

  // RHI implementations
  [[nodiscard]] rhi::Filter GetMagFilter() const override { return magFilter_; }
  [[nodiscard]] rhi::Filter GetMinFilter() const override { return minFilter_; }
  [[nodiscard]] rhi::AddressMode GetAddressModeU() const override {
    return addressModeU_;
  }
  [[nodiscard]] rhi::AddressMode GetAddressModeV() const override {
    return addressModeV_;
  }

  [[nodiscard]] std::span<const float, 4> GetBorderColor() const override {
    return std::span<const float, 4>(glm::value_ptr(borderColor_), 4);
  }

  [[nodiscard]] bool IsCompareEnabled() const override {
    return compareEnable_;
  }

  [[nodiscard]] rhi::CompareOp GetCompareOp() const override {
    return compareOp_;
  }

  // Vulkan getter
  [[nodiscard]] vk::Sampler GetSampler() const { return sampler_.get(); }

 private:
  VulkanSampler(rhi::Filter magFilter, rhi::Filter minFilter,
                rhi::AddressMode addressModeU, rhi::AddressMode addressModeV,
                std::optional<std::span<const float, 4>> borderColor,
                bool compareEnable, rhi::CompareOp compareOp,
                vk::UniqueSampler sampler);

  rhi::Filter magFilter_;
  rhi::Filter minFilter_;
  rhi::AddressMode addressModeU_;
  rhi::AddressMode addressModeV_;
  vk::UniqueSampler sampler_;
  glm::vec4 borderColor_{0.0F};
  bool compareEnable_{false};
  rhi::CompareOp compareOp_{rhi::CompareOp::Always};
};
}  // namespace backends::vulkan
