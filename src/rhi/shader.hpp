#pragma once

#include <cstdint>
#include <vector>

namespace rhi {
/**
 * @brief Shader stages supported by the renderer
 */
enum class ShaderStage : uint8_t {
  Vertex,
  Fragment,
  Compute,
};

/**
 * @brief Abstract base class for shaders
 */
class Shader {
 public:
  virtual ~Shader() = default;

  /**
   * @brief Gets the shader stage
   *
   * @return ShaderStage The shader stage
   */
  [[nodiscard]] virtual ShaderStage GetStage() const = 0;

  /**
   * @brief Gets the SPIR-V code of the shader
   *
   * @return const std::vector<uint32_t>& The SPIR-V code
   */
  [[nodiscard]] virtual const std::vector<uint32_t>& GetSPIRVCode() const = 0;
};
}  // namespace rhi
