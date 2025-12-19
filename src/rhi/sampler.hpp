#pragma once

#include <cstdint>

namespace rhi {
/**
 * @brief Texture sampling filter modes.
 */
enum class Filter : uint8_t {
  Nearest,  // Point sampling
  Linear,   // Bilinear filtering
};

/**
 * @brief Texture addressing modes.
 */
enum class AddressMode : uint8_t {
  Repeat,       // Repeat the texture
  ClampToEdge,  // Clamp to the edge of the texture
};

/**
 * @brief Abstract base class for texture samplers.
 */
class Sampler {
 public:
  virtual ~Sampler() = default;

  /**
   * @brief Gets the magnification filter mode.
   *
   * @return Filter The magnification filter mode.
   */
  [[nodiscard]] virtual Filter GetMagFilter() const = 0;

  /**
   * @brief Gets the minification filter mode.
   *
   * @return Filter The minification filter mode.
   */
  [[nodiscard]] virtual Filter GetMinFilter() const = 0;

  /**
   * @brief Gets the U address mode.
   *
   * @return AddressMode The U address mode.
   */
  [[nodiscard]] virtual AddressMode GetAddressModeU() const = 0;

  /**
   * @brief Gets the V address mode.
   *
   * @return AddressMode The V address mode.
   */
  [[nodiscard]] virtual AddressMode GetAddressModeV() const = 0;

 private:
};
}  // namespace rhi
