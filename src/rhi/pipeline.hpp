#pragma once

#include "rhi/descriptor.hpp"
#include "rhi/shader.hpp"

namespace rhi {
/**
 * @brief Describes a push constant range
 */
struct PushConstantRange {
  ShaderStage stage{ShaderStage::Vertex};
  uint32_t offset{0};
  uint32_t size{0};
};

/**
 * @brief Abstract base class for a pipeline layout.
 */
class PipelineLayout {
 public:
  virtual ~PipelineLayout() = default;

  /**
   * @brief Get the descriptor set layouts associated with the pipeline layout.
   *
   * @return const std::vector<DescriptorSetLayout*>& Reference to the vector of
   * descriptor set layouts.
   */
  [[nodiscard]] virtual auto GetSetLayouts() const
      -> const std::vector<DescriptorSetLayout*>& = 0;

  /**
   * @brief Get the push constant ranges associated with the pipeline layout.
   *
   * @return const std::vector<PushConstantRange>& Reference to the push
   * constant ranges.
   */
  [[nodiscard]] virtual auto GetPushConstantRanges() const
      -> const std::vector<PushConstantRange>& = 0;
};

/**
 * @brief Vertex input rate
 */
enum class VertexInputRate : uint8_t {
  Vertex,
  Instance,
};

/**
 * @brief Describes a vertex binding
 */
struct VertexBinding {
  uint32_t binding{0};
  uint32_t stride{0};
  VertexInputRate inputRate{VertexInputRate::Vertex};
};

/**
 * @brief Describes a vertex attribute
 */
struct VertexAttribute {
  uint32_t location{0};
  uint32_t binding{0};
  Format format{Format::R32G32B32A32Sfloat};
  uint32_t offset{0};
};

/**
 * @brief Face culling mode
 */
enum class CullMode : uint8_t { None, Front, Back };

/**
 * @brief Structure describing the configuration of a graphics pipeline.
 */
struct GraphicsPipelineDesc {
  // Vertex shader
  const Shader* vertexShader{nullptr};

  // Fragment shader
  const Shader* fragmentShader{nullptr};

  // Pipeline layout
  const PipelineLayout* layout{nullptr};

  // Vertex input
  std::span<const VertexBinding> vertexBindings;
  std::span<const VertexAttribute> vertexAttributes;

  // Color attachment formats for dynamic rendering
  std::span<const Format> colorFormats;
  Format depthFormat{Format::Undefined};

  // Depth settings
  bool depthTest{true};
  bool depthWrite{true};

  // Rasterization settings
  CullMode cullMode{CullMode::Back};
  bool wireframe{false};
  bool blendEnabled{false};
};

/**
 * @brief Abstract base class for a rendering pipeline.
 */
class Pipeline {
 public:
  virtual ~Pipeline() = default;

  /**
   * @brief Get the associated shaders of the pipeline.
   *
   * @return const PipelineLayout& Reference to the pipeline layout.
   */
  [[nodiscard]] virtual const PipelineLayout& GetLayout() const = 0;

  /**
   * @brief Check if the pipeline is a graphics pipeline.
   *
   * @return true if the pipeline is a graphics pipeline.
   * @return false otherwise.
   */
  [[nodiscard]] virtual bool IsGraphics() const = 0;
};
}  // namespace rhi
