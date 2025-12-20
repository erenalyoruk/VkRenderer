#pragma once

#include <memory>

#include <vulkan/vulkan.hpp>

#include "rhi/sampler.hpp"

namespace backends::vulkan {
class VulkanContext;

class VulkanSampler : public rhi::Sampler {
 public:
  static std::unique_ptr<VulkanSampler> Create(VulkanContext& context,
                                               rhi::Filter magFilter,
                                               rhi::Filter minFilter,
                                               rhi::AddressMode addressModeU,
                                               rhi::AddressMode addressModeV);

  // RHI implementations
  [[nodiscard]] rhi::Filter GetMagFilter() const override { return magFilter_; }
  [[nodiscard]] rhi::Filter GetMinFilter() const override { return minFilter_; }
  [[nodiscard]] rhi::AddressMode GetAddressModeU() const override {
    return addressModeU_;
  }
  [[nodiscard]] rhi::AddressMode GetAddressModeV() const override {
    return addressModeV_;
  }

  // Vulkan getter
  [[nodiscard]] vk::Sampler GetSampler() const { return sampler_.get(); }

 private:
  VulkanSampler(rhi::Filter magFilter, rhi::Filter minFilter,
                rhi::AddressMode addressModeU, rhi::AddressMode addressModeV,
                vk::UniqueSampler sampler);

  rhi::Filter magFilter_;
  rhi::Filter minFilter_;
  rhi::AddressMode addressModeU_;
  rhi::AddressMode addressModeV_;
  vk::UniqueSampler sampler_;
};
}  // namespace backends::vulkan
