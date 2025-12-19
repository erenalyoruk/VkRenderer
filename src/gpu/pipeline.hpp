#pragma once

#include <vector>

#include <vulkan/vulkan.hpp>

namespace gpu {
class GraphicsPipelineBuilder {
 public:
  GraphicsPipelineBuilder(vk::PipelineLayout layout, vk::Format colorFormat,
                          vk::Format depthFormat = vk::Format::eUndefined);

  GraphicsPipelineBuilder& SetShaders(vk::ShaderModule vertexShader,
                                      vk::ShaderModule fragmentShader);
  GraphicsPipelineBuilder& SetTopology(vk::PrimitiveTopology topology);
  GraphicsPipelineBuilder& SetPolygonMode(vk::PolygonMode polygonMode);
  GraphicsPipelineBuilder& SetCullMode(vk::CullModeFlags cullMode,
                                       vk::FrontFace frontFace);

  GraphicsPipelineBuilder& SetDepthTest(bool enableDepthTest, bool writeEnable,
                                        vk::CompareOp compareOp);
  GraphicsPipelineBuilder& SetBlending(
      bool enableBlending,
      vk::BlendFactor srcFactor = vk::BlendFactor::eSrcAlpha,
      vk::BlendFactor dstFactor = vk::BlendFactor::eOneMinusSrcAlpha);
  GraphicsPipelineBuilder& SetAdditiveBlending();
  GraphicsPipelineBuilder& SetMultisampling(
      vk::SampleCountFlagBits sampleCount = vk::SampleCountFlagBits::e1);

  vk::UniquePipeline Build(vk::Device device);

 private:
  vk::PipelineLayout layout_;
  vk::Format colorFormat_;
  vk::Format depthFormat_;

  std::vector<vk::PipelineShaderStageCreateInfo> shaderStages_;

  vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo_{
      .topology = vk::PrimitiveTopology::eTriangleList,
      .primitiveRestartEnable = VK_FALSE,
  };

  vk::PipelineRasterizationStateCreateInfo rasterizationInfo_{
      .depthClampEnable = VK_FALSE,
      .rasterizerDiscardEnable = VK_FALSE,
      .polygonMode = vk::PolygonMode::eFill,
      .cullMode = vk::CullModeFlagBits::eBack,
      .frontFace = vk::FrontFace::eClockwise,
      .depthBiasEnable = VK_FALSE,
      .lineWidth = 1.0F,
  };

  vk::PipelineMultisampleStateCreateInfo multisamplingInfo_{
      .rasterizationSamples = vk::SampleCountFlagBits::e1,
  };

  vk::PipelineDepthStencilStateCreateInfo depthStencilInfo_{
      .depthTestEnable = VK_FALSE,
      .depthWriteEnable = VK_FALSE,
      .depthCompareOp = vk::CompareOp::eLess,
  };

  vk::PipelineColorBlendAttachmentState colorBlendAttachment_{
      .blendEnable = VK_FALSE,
      .srcColorBlendFactor = vk::BlendFactor::eSrcAlpha,
      .dstColorBlendFactor = vk::BlendFactor::eOne,
      .colorBlendOp = vk::BlendOp::eAdd,
      .srcAlphaBlendFactor = vk::BlendFactor::eOne,
      .dstAlphaBlendFactor = vk::BlendFactor::eZero,
      .alphaBlendOp = vk::BlendOp::eAdd,
      .colorWriteMask =
          vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
          vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
  };

  std::vector<vk::DynamicState> dynamicStates_{vk::DynamicState::eViewport,
                                               vk::DynamicState::eScissor};
};

class ComputePipelineBuilder {
 public:
  explicit ComputePipelineBuilder(vk::PipelineLayout layout);

  ComputePipelineBuilder& SetShader(vk::ShaderModule computeShader);

  vk::UniquePipeline Build(vk::Device device);

 private:
  vk::PipelineLayout layout_;
  vk::PipelineShaderStageCreateInfo shaderStage_;
};
}  // namespace gpu
