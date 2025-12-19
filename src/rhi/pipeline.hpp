#pragma once

#include "rhi/descriptor.hpp"
#include "rhi/shader.hpp"

namespace rhi {
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
};

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
