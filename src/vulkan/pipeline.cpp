#include "vulkan/pipeline.hpp"

#include <stdexcept>

#include "logger.hpp"

namespace vulkan {
GraphicsPipelineBuilder::GraphicsPipelineBuilder(vk::PipelineLayout layout,
                                                 vk::Format colorFormat,
                                                 vk::Format depthFormat)
    : layout_{layout}, colorFormat_{colorFormat}, depthFormat_{depthFormat} {}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetShaders(
    vk::ShaderModule vertexShader, vk::ShaderModule fragmentShader) {
  shaderStages_.clear();
  shaderStages_.push_back(vk::PipelineShaderStageCreateInfo{
      .stage = vk::ShaderStageFlagBits::eVertex,
      .module = vertexShader,
      .pName = "main",
  });

  shaderStages_.push_back(vk::PipelineShaderStageCreateInfo{
      .stage = vk::ShaderStageFlagBits::eFragment,
      .module = fragmentShader,
      .pName = "main",
  });

  return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetTopology(
    vk::PrimitiveTopology topology) {
  inputAssemblyInfo_.topology = topology;
  return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetPolygonMode(
    vk::PolygonMode polygonMode) {
  rasterizationInfo_.polygonMode = polygonMode;
  return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetCullMode(
    vk::CullModeFlags cullMode, vk::FrontFace frontFace) {
  rasterizationInfo_.cullMode = cullMode;
  rasterizationInfo_.frontFace = frontFace;
  return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetDepthTest(
    bool enableDepthTest, bool writeEnable, vk::CompareOp depthCompareOp) {
  depthStencilInfo_.depthTestEnable = enableDepthTest ? VK_TRUE : VK_FALSE;
  depthStencilInfo_.depthWriteEnable = writeEnable ? VK_TRUE : VK_FALSE;
  depthStencilInfo_.depthCompareOp = depthCompareOp;
  return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetBlending(
    bool enableBlending, vk::BlendFactor srcFactor, vk::BlendFactor dstFactor) {
  colorBlendAttachment_.blendEnable = enableBlending ? VK_TRUE : VK_FALSE;
  colorBlendAttachment_.srcColorBlendFactor = srcFactor;
  colorBlendAttachment_.dstColorBlendFactor = dstFactor;
  return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetMultisampling(
    vk::SampleCountFlagBits sampleCount) {
  multisamplingInfo_.rasterizationSamples = sampleCount;
  return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetAdditiveBlending() {
  return SetBlending(true, vk::BlendFactor::eOne, vk::BlendFactor::eOne);
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetVertexInput(
    const std::vector<vk::VertexInputBindingDescription>& bindings,
    const std::vector<vk::VertexInputAttributeDescription>& attributes) {
  vertexBindings_ = bindings;
  vertexAttributes_ = attributes;
  return *this;
}

vk::UniquePipeline GraphicsPipelineBuilder::Build(vk::Device device) {
  vk::PipelineVertexInputStateCreateInfo vertexInputInfo{
      .vertexBindingDescriptionCount =
          static_cast<uint32_t>(vertexBindings_.size()),
      .pVertexBindingDescriptions = vertexBindings_.data(),
      .vertexAttributeDescriptionCount =
          static_cast<uint32_t>(vertexAttributes_.size()),
      .pVertexAttributeDescriptions = vertexAttributes_.data(),
  };

  vk::PipelineViewportStateCreateInfo viewportInfo{
      .viewportCount = 1,
      .scissorCount = 1,
  };

  vk::PipelineColorBlendStateCreateInfo colorBlendInfo{
      .logicOpEnable = VK_FALSE,
      .logicOp = vk::LogicOp::eCopy,
      .attachmentCount = 1,
      .pAttachments = &colorBlendAttachment_,
  };

  vk::PipelineDynamicStateCreateInfo dynamicStateInfo{
      .dynamicStateCount = static_cast<uint32_t>(dynamicStates_.size()),
      .pDynamicStates = dynamicStates_.data(),
  };

  vk::PipelineRenderingCreateInfo renderingInfo{
      .viewMask = 0,
      .colorAttachmentCount = 1,
      .pColorAttachmentFormats = &colorFormat_,
      .depthAttachmentFormat = depthFormat_ != vk::Format::eUndefined
                                   ? depthFormat_
                                   : vk::Format::eUndefined,
      .stencilAttachmentFormat = vk::Format::eUndefined,
  };

  vk::GraphicsPipelineCreateInfo pipelineInfo{
      .pNext = &renderingInfo,
      .stageCount = static_cast<uint32_t>(shaderStages_.size()),
      .pStages = shaderStages_.data(),
      .pVertexInputState = &vertexInputInfo,
      .pInputAssemblyState = &inputAssemblyInfo_,
      .pViewportState = &viewportInfo,
      .pRasterizationState = &rasterizationInfo_,
      .pMultisampleState = &multisamplingInfo_,
      .pDepthStencilState = &depthStencilInfo_,
      .pColorBlendState = &colorBlendInfo,
      .pDynamicState = &dynamicStateInfo,
      .layout = layout_,
      .subpass = 0,
  };

  auto result{device.createGraphicsPipelineUnique(nullptr, pipelineInfo)};
  if (result.result != vk::Result::eSuccess) {
    LOG_CRITICAL("Failed to create graphics pipeline!");
    throw std::runtime_error{"Failed to create graphics pipeline!"};
  }

  return std::move(result.value);
}

ComputePipelineBuilder::ComputePipelineBuilder(vk::PipelineLayout layout)
    : layout_{layout} {}

ComputePipelineBuilder& ComputePipelineBuilder::SetShader(
    vk::ShaderModule computeShader) {
  shaderStage_ = vk::PipelineShaderStageCreateInfo{
      .stage = vk::ShaderStageFlagBits::eCompute,
      .module = computeShader,
      .pName = "main",
  };
  return *this;
}

vk::UniquePipeline ComputePipelineBuilder::Build(vk::Device device) {
  vk::ComputePipelineCreateInfo pipelineInfo{
      .stage = shaderStage_,
      .layout = layout_,
  };

  auto result{device.createComputePipelineUnique(nullptr, pipelineInfo)};
  if (result.result != vk::Result::eSuccess) {
    LOG_CRITICAL("Failed to create compute pipeline!");
    throw std::runtime_error{"Failed to create compute pipeline!"};
  }

  return std::move(result.value);
}
}  // namespace vulkan
