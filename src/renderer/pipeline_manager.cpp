#include "renderer/pipeline_manager.hpp"

#include <array>

#include "ecs/components.hpp"
#include "logger.hpp"
#include "rhi/shader_utils.hpp"

namespace renderer {

PipelineManager::PipelineManager(rhi::Factory& factory, rhi::Device& device)
    : factory_{factory}, device_{device} {}

void PipelineManager::Initialize(rhi::DescriptorSetLayout* globalLayout,
                                 rhi::DescriptorSetLayout* materialLayout,
                                 rhi::DescriptorSetLayout* iblLayout,
                                 rhi::DescriptorSetLayout* objectLayout) {
  globalLayout_ = globalLayout;
  materialLayout_ = materialLayout;
  iblLayout_ = iblLayout;
  objectLayout_ = objectLayout;

  // Create pipeline layout with all descriptor sets
  // Set 0: Global
  // Set 1: Materials
  // Set 2: Objects
  // Set 3: IBL
  std::vector<rhi::DescriptorSetLayout*> layouts = {
      globalLayout_,    // set 0
      materialLayout_,  // set 1
      objectLayout_,    // set 2
      iblLayout_,       // set 3
  };

  std::array<rhi::PushConstantRange, 1> pushConstants = {{
      {.stage = rhi::ShaderStage::Vertex, .offset = 0, .size = 128},
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
                     .depthTest = true,
                     .depthWrite = true,
                     .doubleSided = true,
                     .wireframe = true,
                     .blendEnabled = false,
                 });

  // Skybox only uses position
  std::vector<rhi::VertexBinding> skyboxBindings = {{
      .binding = 0,
      .stride = sizeof(ecs::Vertex),
      .inputRate = rhi::VertexInputRate::Vertex,
  }};

  std::vector<rhi::VertexAttribute> skyboxAttributes = {{
      .location = 0,
      .binding = 0,
      .format = rhi::Format::R32G32B32Sfloat,
      .offset = offsetof(ecs::Vertex, position),
  }};

  CreatePipeline(PipelineType::Skybox,
                 {
                     .vertexShaderPath = "assets/shaders/skybox.vert.spv",
                     .fragmentShaderPath = "assets/shaders/skybox.frag.spv",
                     .depthTest = true,
                     .depthWrite = false,
                     .depthCompareOp = rhi::CompareOp::LessOrEqual,
                     .doubleSided = false,
                     .wireframe = false,
                     .blendEnabled = false,
                     .vertexBindings = skyboxBindings,
                     .vertexAttributes = skyboxAttributes,
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

  // Use custom vertex layout if provided, otherwise use default Vertex
  std::vector<rhi::VertexBinding> bindings;
  std::vector<rhi::VertexAttribute> attributes;

  if (config.vertexBindings && config.vertexAttributes) {
    bindings = *config.vertexBindings;
    attributes = *config.vertexAttributes;
  } else {
    bindings = ecs::Vertex::GetBindings();
    attributes = ecs::Vertex::GetAttributes();
  }

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
      .depthCompareOp = config.depthCompareOp,
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
  Initialize(globalLayout_, materialLayout_, objectLayout_);
}

}  // namespace renderer
