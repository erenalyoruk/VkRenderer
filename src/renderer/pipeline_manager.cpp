#include "renderer/pipeline_manager.hpp"

#include <array>

#include "ecs/components.hpp"
#include "logger.hpp"
#include "rhi/shader_utils.hpp"

namespace renderer {

PipelineManager::PipelineManager(rhi::Factory& factory, rhi::Device& device)
    : factory_{factory}, device_{device} {}

void PipelineManager::Initialize(rhi::DescriptorSetLayout* globalLayout,
                                 rhi::DescriptorSetLayout* materialLayout) {
  globalLayout_ = globalLayout;
  materialLayout_ = materialLayout;

  // Create pipeline layout
  std::array<rhi::DescriptorSetLayout*, 2> layouts = {
      globalLayout_,
      materialLayout_,
  };

  std::array<rhi::PushConstantRange, 1> pushConstants = {{
      {.stage = rhi::ShaderStage::Vertex, .offset = 0, .size = 128},  // 2x mat4
  }};

  pipelineLayout_ = factory_.CreatePipelineLayout(layouts, pushConstants);

  // Create default pipelines
  CreatePipeline(PipelineType::PBRLit,
                 {
                     .vertexShaderPath = "assets/shaders/pbr.vert.spv",
                     .fragmentShaderPath = "assets/shaders/pbr.frag.spv",
                 });

  CreatePipeline(PipelineType::Unlit,
                 {
                     .vertexShaderPath = "assets/shaders/unlit.vert.spv",
                     .fragmentShaderPath = "assets/shaders/unlit.frag.spv",
                 });

  CreatePipeline(PipelineType::Wireframe,
                 {
                     .vertexShaderPath = "assets/shaders/wireframe.vert.spv",
                     .fragmentShaderPath = "assets/shaders/wireframe.frag.spv",
                     .doubleSided = true,
                     .wireframe = true,
                 });
}

void PipelineManager::CreatePipeline(PipelineType type,
                                     const PipelineConfig& config) {
  auto vertShader = rhi::CreateShaderFromFile(factory_, config.vertexShaderPath,
                                              rhi::ShaderStage::Vertex);
  auto fragShader = rhi::CreateShaderFromFile(
      factory_, config.fragmentShaderPath, rhi::ShaderStage::Fragment);

  if (!vertShader || !fragShader) {
    LOG_WARNING("Failed to load shaders for pipeline: {} / {}",
                config.vertexShaderPath, config.fragmentShaderPath);
    return;
  }

  auto bindings = ecs::Vertex::GetBindings();
  auto attributes = ecs::Vertex::GetAttributes();

  auto* swapchain = device_.GetSwapchain();
  rhi::Format colorFormat = swapchain->GetImages()[0]->GetFormat();

  std::array<rhi::Format, 1> colorFormats = {colorFormat};

  rhi::GraphicsPipelineDesc pipelineDesc{
      .vertexShader = vertShader.get(),
      .fragmentShader = fragShader.get(),
      .layout = pipelineLayout_.get(),
      .vertexBindings = bindings,
      .vertexAttributes = attributes,
      .colorFormats = colorFormats,
      .depthFormat = rhi::Format::D32Sfloat,
      .depthTest = config.depthTest,
      .depthWrite = config.depthWrite,
      .cullMode =
          config.doubleSided ? rhi::CullMode::None : rhi::CullMode::Back,
      .wireframe = config.wireframe,
      .blendEnabled = config.blendEnabled,
  };

  auto pipeline = factory_.CreateGraphicsPipeline(pipelineDesc);
  if (pipeline) {
    pipelines_[type] = std::move(pipeline);
    LOG_INFO("Created pipeline: {}", config.vertexShaderPath);
  }
}

rhi::Pipeline* PipelineManager::GetPipeline(PipelineType type) {
  auto it = pipelines_.find(type);
  if (it != pipelines_.end()) {
    return it->second.get();
  }
  return nullptr;
}

void PipelineManager::RecreatePipelines() {
  pipelines_.clear();
  Initialize(globalLayout_, materialLayout_);
}

}  // namespace renderer
