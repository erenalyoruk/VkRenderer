#pragma once

#include <cstdint>
#include <span>

#include "rhi/types.hpp"

namespace rhi {
/**
 * @brief Usage flags for textures
 */
enum class TextureUsage : uint8_t {
  Sampled = 1 << 0,                 // Sampled texture
  Storage = 1 << 1,                 // Storage texture
  ColorAttachment = 1 << 2,         // Color attachment
  DepthStencilAttachment = 1 << 3,  // Depth-stencil attachment
};

constexpr TextureUsage operator|(TextureUsage a, TextureUsage b) {
  return static_cast<TextureUsage>(static_cast<uint8_t>(a) |
                                   static_cast<uint8_t>(b));
}

constexpr TextureUsage operator&(TextureUsage a, TextureUsage b) {
  return static_cast<TextureUsage>(static_cast<uint8_t>(a) &
                                   static_cast<uint8_t>(b));
}

enum class ImageLayout : uint8_t {
  Undefined,
  General,
  ColorAttachment,
  DepthStencilAttachment,
  ShaderReadOnly,
  TransferSrc,
  TransferDst,
  Present,
};

/**
 * @brief Represents a GPU texture resource.
 */
class Texture {
 public:
  virtual ~Texture() = default;

  /**
   * @brief Uploads data to the texture.
   *
   * @param data The data to upload.
   * @param mipLevel Mip level to upload to.
   * @param arrayLayer Array layer to upload to.
   */
  virtual void Upload(std::span<const std::byte> data, uint32_t mipLevel = 0,
                      uint32_t arrayLayer = 0) = 0;

  /**
   * @brief Gets the format of the texture.
   *
   * @return Format Format of the texture.
   */
  [[nodiscard]] virtual Format GetFormat() const = 0;

  /**
   * @brief Gets the width of the texture.
   *
   * @return uint32_t Width of the texture.
   */
  [[nodiscard]] virtual uint32_t GetWidth() const = 0;

  /**
   * @brief Gets the height of the texture.
   *
   * @return uint32_t Height of the texture.
   */
  [[nodiscard]] virtual uint32_t GetHeight() const = 0;

  /**
   * @brief Gets the depth of the texture.
   *
   * @return uint32_t Depth of the texture.
   */
  [[nodiscard]] virtual uint32_t GetDepth() const = 0;

  /**
   * @brief Gets the number of mip levels of the texture.
   *
   * @return uint32_t Number of mip levels.
   */
  [[nodiscard]] virtual uint32_t GetMipLevels() const = 0;
};
}  // namespace rhi
