#include "vulkan/descriptors.hpp"

#include <utility>

#include "logger.hpp"

namespace vulkan {
DescriptorLayoutBuilder& DescriptorLayoutBuilder::AddBinding(
    uint32_t binding, vk::DescriptorType descriptorType,
    vk::ShaderStageFlags stageFlags, uint32_t count, bool variableCount) {
  vk::DescriptorSetLayoutBinding layoutBinding{
      .binding = binding,
      .descriptorType = descriptorType,
      .descriptorCount = count,
      .stageFlags = stageFlags,
  };

  bindings_.push_back(layoutBinding);

  vk::DescriptorBindingFlags flags{};
  if (variableCount) {
    flags |= vk::DescriptorBindingFlagBits::eVariableDescriptorCount |
             vk::DescriptorBindingFlagBits::ePartiallyBound;
  }

  bindingFlags_.push_back(flags);
  return *this;
}

vk::UniqueDescriptorSetLayout DescriptorLayoutBuilder::Build(
    vk::Device device, vk::DescriptorSetLayoutCreateFlags flags) {
  vk::DescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsInfo{
      .bindingCount = static_cast<uint32_t>(bindingFlags_.size()),
      .pBindingFlags = bindingFlags_.data(),
  };

  return device.createDescriptorSetLayoutUnique({
      .pNext = &bindingFlagsInfo,
      .flags = flags,
      .bindingCount = static_cast<uint32_t>(bindings_.size()),
      .pBindings = bindings_.data(),
  });
}

DescriptorAllocator::DescriptorAllocator(
    vk::Device device, uint32_t maxSets,
    std::vector<PoolSizeRatio> poolSizeRatios)
    : device_{device},
      setsPerPool_{maxSets},
      poolSizeRatios_{std::move(poolSizeRatios)} {
  auto newPool{CreatePool(setsPerPool_, poolSizeRatios_)};
  currentPool_ = newPool.get();
  usedPools_.push_back(std::move(newPool));
}

vk::DescriptorSet DescriptorAllocator::Allocate(
    vk::DescriptorSetLayout layout) {
  vk::DescriptorPool pool{GetPool()};

  vk::DescriptorSetAllocateInfo allocInfo{
      .descriptorPool = pool,
      .descriptorSetCount = 1,
      .pSetLayouts = &layout,
  };

  vk::DescriptorSet descriptorSet;
  vk::Result result{device_.allocateDescriptorSets(&allocInfo, &descriptorSet)};

  if (result == vk::Result::eErrorOutOfPoolMemory ||
      result == vk::Result::eErrorFragmentedPool) {
    currentPool_ = nullptr;
    pool = GetPool();
    allocInfo.descriptorPool = pool;

    result = device_.allocateDescriptorSets(&allocInfo, &descriptorSet);
    if (result != vk::Result::eSuccess) {
      LOG_CRITICAL("Failed to allocate descriptor set after retry!");
      throw std::runtime_error(
          "Failed to allocate descriptor set after retry!");
    }
  }

  return descriptorSet;
}

void DescriptorAllocator::ResetPools() {
  for (auto& pool : usedPools_) {
    device_.resetDescriptorPool(pool.get());
    freePools_.push_back(std::move(pool));
  }

  usedPools_.clear();
  currentPool_ = nullptr;
}

vk::UniqueDescriptorPool DescriptorAllocator::CreatePool(
    uint32_t setCount, std::span<const PoolSizeRatio> ratios) {
  std::vector<vk::DescriptorPoolSize> poolSizes;
  for (const auto& ratio : ratios) {
    poolSizes.emplace_back(ratio.type,
                           static_cast<uint32_t>(ratio.ratio) * setCount);
  }

  vk::DescriptorPoolCreateInfo poolInfo{
      .maxSets = setCount,
      .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
      .pPoolSizes = poolSizes.data(),
  };

  return device_.createDescriptorPoolUnique(poolInfo);
}

vk::DescriptorPool DescriptorAllocator::GetPool() {
  if (currentPool_ == nullptr) {
    if (!freePools_.empty()) {
      currentPool_ = freePools_.back().get();
      usedPools_.push_back(std::move(freePools_.back()));
      freePools_.pop_back();
    } else {
      auto newPool{CreatePool(setsPerPool_, poolSizeRatios_)};
      currentPool_ = newPool.get();
      usedPools_.push_back(std::move(newPool));
    }
  }

  return currentPool_;
}

DescriptorWriter& DescriptorWriter::WriteBuffer(
    uint32_t binding, vk::DescriptorType type,
    const vk::DescriptorBufferInfo& bufferInfo) {
  bufferInfos_.push_back(bufferInfo);

  vk::WriteDescriptorSet write{
      .dstBinding = binding,
      .dstArrayElement = 0,
      .descriptorCount = 1,
      .descriptorType = type,
      .pBufferInfo = &bufferInfos_.back(),
  };

  writes_.push_back(write);
  return *this;
}

DescriptorWriter& DescriptorWriter::WriteImage(
    uint32_t binding, vk::DescriptorType type,
    const vk::DescriptorImageInfo& imageInfo) {
  imageInfos_.push_back(imageInfo);

  vk::WriteDescriptorSet write{
      .dstBinding = binding,
      .dstArrayElement = 0,
      .descriptorCount = 1,
      .descriptorType = type,
      .pImageInfo = &imageInfos_.back(),
  };

  writes_.push_back(write);
  return *this;
}

void DescriptorWriter::Overwrite(vk::Device device,
                                 vk::DescriptorSet descriptorSet) {
  for (auto& write : writes_) {
    write.dstSet = descriptorSet;
  }

  device.updateDescriptorSets(writes_, {});
}
}  // namespace vulkan
