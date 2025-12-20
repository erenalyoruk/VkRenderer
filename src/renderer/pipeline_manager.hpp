#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

#include "rhi/device.hpp"
#include "rhi/factory.hpp"
#include "rhi/pipeline.hpp"

namespace renderer {

enum class PipelineType : uint8_t {
  PBRLit,
  Unlit,
  Wireframe,
  ShadowMap,
  Count
};

struct PipelineConfig {
  std::string vertexShaderPath;
  std::string fragmentShaderPath;
  bool depthTest{true};
  bool depthWrite{true};
  bool doubleSided{false};
  bool wireframe{false};
  bool blendEnabled{false};
};

class PipelineManager {
 public:
  PipelineManager(rhi::Factory& factory, rhi::Device& device);

  void Initialize(rhi::DescriptorSetLayout* globalLayout,
                  rhi::DescriptorSetLayout* materialLayout,
                  rhi::DescriptorSetLayout* objectLayout = nullptr);

  [[nodiscard]] rhi::Pipeline* GetPipeline(PipelineType type);
  [[nodiscard]] rhi::PipelineLayout* GetPipelineLayout() {
    return pipelineLayout_.get();
  }

  void RecreatePipelines();

 private:
  void CreatePipeline(PipelineType type, const PipelineConfig& config);

  rhi::Factory& factory_;
  rhi::Device& device_;

  std::unique_ptr<rhi::PipelineLayout> pipelineLayout_;
  std::unordered_map<PipelineType, std::unique_ptr<rhi::Pipeline>> pipelines_;

  rhi::DescriptorSetLayout* globalLayout_{nullptr};
  rhi::DescriptorSetLayout* materialLayout_{nullptr};
  rhi::DescriptorSetLayout* objectLayout_{nullptr};
};

}  // namespace renderer
