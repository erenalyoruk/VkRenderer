#pragma once

#include <cstdint>
#include <vector>

#include "rhi/buffer.hpp"
#include "rhi/sampler.hpp"
#include "rhi/texture.hpp"

namespace rhi {
/**
 * @brief Types of descriptors that can be used in descriptor sets.
 */
enum class DescriptorType : uint8_t {
  UniformBuffer,         // UBO
  StorageBuffer,         // SSBO
  SampledImage,          // Sampled Texture (separate from sampler)
  Sampler,               // Sampler only
  CombinedImageSampler,  // Texture + Sampler combined
  StorageImage,          // Storage image for compute
};

/**
 * @brief Represents a single binding within a descriptor set layout.
 */
struct DescriptorBinding {
  // The binding index within the descriptor set.
  uint32_t binding{0};

  // The type of descriptor (e.g., UniformBuffer, StorageBuffer, etc.).
  DescriptorType type{DescriptorType::UniformBuffer};

  // The number of descriptors for this binding (for arrays).
  uint32_t count{1};
};

/**
 * @brief Represents the layout of a descriptor set, defining its bindings.
 */
class DescriptorSetLayout {
 public:
  virtual ~DescriptorSetLayout() = default;

  /**
   * @brief Gets the list of descriptor bindings in this layout.
   *
   * @return const std::vector<DescriptorBinding>& The descriptor bindings.
   */
  [[nodiscard]] virtual const std::vector<DescriptorBinding>& GetBindings()
      const = 0;
};

/**
 * @brief Represents a descriptor set that holds actual resource bindings.
 */
class DescriptorSet {
 public:
  virtual ~DescriptorSet() = default;

  /**
   * @brief Binds a buffer to a specific binding point in the descriptor set.
   *
   * @param binding Binding index to bind the buffer to.
   * @param buffer Pointer to the buffer to bind.
   * @param offset Offset within the buffer to start binding from.
   * @param range Range of the buffer to bind. If 0, binds to the end of the
   * buffer.
   */
  virtual void BindBuffer(uint32_t binding, const Buffer* buffer,
                          Size offset = 0, Size range = 0) = 0;

  /**
   * @brief Binds a storage buffer to a descriptor set.
   *
   * @param binding Binding index to bind the storage buffer to.
   * @param buffer Pointer to the storage buffer to bind.
   * @param offset Offset within the buffer to start binding from.
   * @param range Range of the buffer to bind. If 0, binds to the end of the
   */
  virtual void BindStorageBuffer(uint32_t binding, const Buffer* buffer,
                                 Size offset = 0, Size range = 0) = 0;

  /**
   * @brief Binds a texture (and optional sampler) to a specific binding point
   * in the descriptor set.
   *
   * @param binding Binding index to bind the texture to.
   * @param texture Pointer to the texture to bind.
   * @param sampler Optional pointer to the sampler to bind.
   * @param arrayElement Array element index for descriptor arrays (default 0).
   */
  virtual void BindTexture(uint32_t binding, const Texture* texture,
                           const Sampler* sampler = nullptr,
                           uint32_t arrayElement = 0) = 0;
};
}  // namespace rhi
