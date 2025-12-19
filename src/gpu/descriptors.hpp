#pragma once

#include <cstdint>
#include <deque>
#include <span>
#include <vector>

#include <vulkan/vulkan.hpp>

namespace gpu {
class DescriptorLayoutBuilder {
 public:
  DescriptorLayoutBuilder& AddBinding(
      uint32_t binding, vk::DescriptorType type,
      vk::ShaderStageFlags stageFlags = vk::ShaderStageFlagBits::eAll,
      uint32_t count = 1);

  vk::UniqueDescriptorSetLayout Build(
      vk::Device device, vk::DescriptorSetLayoutCreateFlags flags = {});

 private:
  std::vector<vk::DescriptorSetLayoutBinding> bindings_;
};

class DescriptorAllocator {
 public:
  struct PoolSizeRatio {
    vk::DescriptorType type;
    float ratio;
  };

  DescriptorAllocator(vk::Device device, uint32_t maxSets,
                      std::vector<PoolSizeRatio> poolSizeRatios);

  vk::DescriptorSet Allocate(vk::DescriptorSetLayout layout);

  void ResetPools();

 private:
  vk::Device device_;
  vk::DescriptorPool currentPool_;
  std::vector<vk::UniqueDescriptorPool> usedPools_;
  std::deque<vk::UniqueDescriptorPool> freePools_;

  std::vector<PoolSizeRatio> poolSizeRatios_;
  uint32_t setsPerPool_;

  vk::UniqueDescriptorPool CreatePool(uint32_t setCount,
                                      std::span<const PoolSizeRatio> ratios);
  vk::DescriptorPool GetPool();
};

class DescriptorWriter {
 public:
  DescriptorWriter& WriteBuffer(uint32_t binding, vk::DescriptorType type,
                                const vk::DescriptorBufferInfo& bufferInfo);
  DescriptorWriter& WriteImage(uint32_t binding, vk::DescriptorType type,
                               const vk::DescriptorImageInfo& imageInfo);

  void Overwrite(vk::Device device, vk::DescriptorSet descriptorSet);

 private:
  std::deque<vk::DescriptorImageInfo> imageInfos_;
  std::deque<vk::DescriptorBufferInfo> bufferInfos_;
  std::vector<vk::WriteDescriptorSet> writes_;
};
}  // namespace gpu
