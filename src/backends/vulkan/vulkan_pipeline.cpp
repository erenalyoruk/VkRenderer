#include "backends/vulkan/vulkan_pipeline.hpp"

#include <bit>
#include <utility>

#include "backends/vulkan/vulkan_context.hpp"
#include "backends/vulkan/vulkan_descriptor.hpp"
#include "backends/vulkan/vulkan_shader.hpp"

namespace backends::vulkan {

namespace {
vk::Format ToVkFormat(rhi::Format format) {
  switch (format) {
    case rhi::Format::R8Unorm:
      return vk::Format::eR8Unorm;
    case rhi::Format::R8G8Unorm:
      return vk::Format::eR8G8Unorm;
    case rhi::Format::R8G8B8Unorm:
      return vk::Format::eR8G8B8Unorm;
    case rhi::Format::R8G8B8A8Unorm:
      return vk::Format::eR8G8B8A8Unorm;
    case rhi::Format::R8G8B8A8Srgb:
      return vk::Format::eR8G8B8A8Srgb;
    case rhi::Format::B8G8R8A8Unorm:
      return vk::Format::eB8G8R8A8Unorm;
    case rhi::Format::B8G8R8A8Srgb:
      return vk::Format::eB8G8R8A8Srgb;
    case rhi::Format::R16Sfloat:
      return vk::Format::eR16Sfloat;
    case rhi::Format::R16G16Sfloat:
      return vk::Format::eR16G16Sfloat;
    case rhi::Format::R16G16B16A16Sfloat:
      return vk::Format::eR16G16B16A16Sfloat;
    case rhi::Format::R32Sfloat:
      return vk::Format::eR32Sfloat;
    case rhi::Format::R32G32Sfloat:
      return vk::Format::eR32G32Sfloat;
    case rhi::Format::R32G32B32Sfloat:
      return vk::Format::eR32G32B32Sfloat;
    case rhi::Format::R32G32B32A32Sfloat:
      return vk::Format::eR32G32B32A32Sfloat;
    case rhi::Format::D16Unorm:
      return vk::Format::eD16Unorm;
    case rhi::Format::D32Sfloat:
      return vk::Format::eD32Sfloat;
    case rhi::Format::D24UnormS8Uint:
      return vk::Format::eD24UnormS8Uint;
    case rhi::Format::D32SfloatS8Uint:
      return vk::Format::eD32SfloatS8Uint;
    default:
      return vk::Format::eUndefined;
  }
}

vk::ShaderStageFlags ToVkShaderStage(rhi::ShaderStage stage) {
  switch (stage) {
    case rhi::ShaderStage::Vertex:
      return vk::ShaderStageFlagBits::eVertex;
    case rhi::ShaderStage::Fragment:
      return vk::ShaderStageFlagBits::eFragment;
    case rhi::ShaderStage::Compute:
      return vk::ShaderStageFlagBits::eCompute;
    default:
      return vk::ShaderStageFlagBits::eAll;
  }
}

vk::CullModeFlags ToVkCullMode(rhi::CullMode mode) {
  switch (mode) {
    case rhi::CullMode::None:
      return vk::CullModeFlagBits::eNone;
    case rhi::CullMode::Front:
      return vk::CullModeFlagBits::eFront;
    case rhi::CullMode::Back:
    default:
      return vk::CullModeFlagBits::eBack;
  }
}
}  // namespace

std::unique_ptr<VulkanPipelineLayout> VulkanPipelineLayout::Create(
    VulkanContext& context,
    std::span<const rhi::DescriptorSetLayout* const> setLayouts,
    std::span<const rhi::PushConstantRange> pushConstantRanges) {
  std::vector<vk::DescriptorSetLayout> vkLayouts;
  std::vector<rhi::DescriptorSetLayout*> layoutsCopy;
  for (const auto* layout : setLayouts) {
    const auto* vkLayout =
        std::bit_cast<const VulkanDescriptorSetLayout*>(layout);
    vkLayouts.push_back(vkLayout->GetLayout());
    layoutsCopy.push_back(std::bit_cast<rhi::DescriptorSetLayout*>(layout));
  }

  std::vector<vk::PushConstantRange> vkPushConstantRanges;
  std::vector<rhi::PushConstantRange> pushConstantRangesCopy;
  for (const auto& range : pushConstantRanges) {
    vkPushConstantRanges.push_back({
        .stageFlags = ToVkShaderStage(range.stage),
        .offset = range.offset,
        .size = range.size,
    });
    pushConstantRangesCopy.push_back(range);
  }

  vk::PipelineLayoutCreateInfo layoutInfo{
      .setLayoutCount = static_cast<uint32_t>(vkLayouts.size()),
      .pSetLayouts = vkLayouts.data(),
      .pushConstantRangeCount =
          static_cast<uint32_t>(vkPushConstantRanges.size()),
      .pPushConstantRanges = vkPushConstantRanges.data(),
  };

  vk::UniquePipelineLayout pipelineLayout =
      context.GetDevice().createPipelineLayoutUnique(layoutInfo);

  return std::unique_ptr<VulkanPipelineLayout>(new VulkanPipelineLayout(
      std::move(layoutsCopy), std::move(pushConstantRangesCopy),
      std::move(pipelineLayout)));
}

VulkanPipelineLayout::VulkanPipelineLayout(
    std::vector<rhi::DescriptorSetLayout*> setLayouts,
    std::vector<rhi::PushConstantRange> pushConstantRanges,
    vk::UniquePipelineLayout layout)
    : setLayouts_{std::move(setLayouts)},
      pushConstantRanges_{std::move(pushConstantRanges)},
      layout_{std::move(layout)} {}

std::unique_ptr<VulkanPipeline> VulkanPipeline::Create(
    VulkanContext& context, const rhi::GraphicsPipelineDesc& desc) {
  const auto* vkVertexShader =
      std::bit_cast<const VulkanShader*>(desc.vertexShader);
  const auto* vkFragmentShader =
      std::bit_cast<const VulkanShader*>(desc.fragmentShader);
  const auto* vkLayout =
      std::bit_cast<const VulkanPipelineLayout*>(desc.layout);

  std::vector<vk::PipelineShaderStageCreateInfo> shaderStages{
      {
          .stage = vk::ShaderStageFlagBits::eVertex,
          .module = vkVertexShader->GetShaderModule(),
          .pName = "main",
      },
      {
          .stage = vk::ShaderStageFlagBits::eFragment,
          .module = vkFragmentShader->GetShaderModule(),
          .pName = "main",
      },
  };

  // Build vertex input state from desc
  std::vector<vk::VertexInputBindingDescription> bindingDescs;
  for (const auto& binding : desc.vertexBindings) {
    bindingDescs.push_back({
        .binding = binding.binding,
        .stride = binding.stride,
        .inputRate = binding.inputRate == rhi::VertexInputRate::Vertex
                         ? vk::VertexInputRate::eVertex
                         : vk::VertexInputRate::eInstance,
    });
  }

  std::vector<vk::VertexInputAttributeDescription> attributeDescs;
  for (const auto& attr : desc.vertexAttributes) {
    attributeDescs.push_back({
        .location = attr.location,
        .binding = attr.binding,
        .format = ToVkFormat(attr.format),
        .offset = attr.offset,
    });
  }

  vk::PipelineVertexInputStateCreateInfo vertexInputInfo{
      .vertexBindingDescriptionCount =
          static_cast<uint32_t>(bindingDescs.size()),
      .pVertexBindingDescriptions = bindingDescs.data(),
      .vertexAttributeDescriptionCount =
          static_cast<uint32_t>(attributeDescs.size()),
      .pVertexAttributeDescriptions = attributeDescs.data(),
  };

  vk::PipelineInputAssemblyStateCreateInfo inputAssembly{
      .topology = vk::PrimitiveTopology::eTriangleList,
  };

  vk::PipelineViewportStateCreateInfo viewportState{
      .viewportCount = 1,
      .scissorCount = 1,
  };

  // Use new pipeline desc fields
  vk::PipelineRasterizationStateCreateInfo rasterizer{
      .polygonMode =
          desc.wireframe ? vk::PolygonMode::eLine : vk::PolygonMode::eFill,
      .cullMode = ToVkCullMode(desc.cullMode),
      .frontFace = vk::FrontFace::eCounterClockwise,
      .lineWidth = 1.0F,
  };

  vk::PipelineMultisampleStateCreateInfo multisampling{
      .rasterizationSamples = vk::SampleCountFlagBits::e1,
  };

  // Use depth test/write from desc
  vk::PipelineDepthStencilStateCreateInfo depthStencil{
      .depthTestEnable = static_cast<vk::Bool32>(
          desc.depthTest && desc.depthFormat != rhi::Format::Undefined),
      .depthWriteEnable = static_cast<vk::Bool32>(
          desc.depthWrite && desc.depthFormat != rhi::Format::Undefined),
      .depthCompareOp = vk::CompareOp::eLess,
  };

  // Use blend enabled from desc
  vk::PipelineColorBlendAttachmentState colorBlendAttachment{
      .blendEnable = static_cast<vk::Bool32>(desc.blendEnabled),
      .srcColorBlendFactor = vk::BlendFactor::eSrcAlpha,
      .dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
      .colorBlendOp = vk::BlendOp::eAdd,
      .srcAlphaBlendFactor = vk::BlendFactor::eOne,
      .dstAlphaBlendFactor = vk::BlendFactor::eZero,
      .alphaBlendOp = vk::BlendOp::eAdd,
      .colorWriteMask =
          vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
          vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
  };

  std::vector<vk::PipelineColorBlendAttachmentState> colorBlendAttachments(
      desc.colorFormats.size(), colorBlendAttachment);

  vk::PipelineColorBlendStateCreateInfo colorBlending{
      .attachmentCount = static_cast<uint32_t>(colorBlendAttachments.size()),
      .pAttachments = colorBlendAttachments.data(),
  };

  std::vector<vk::DynamicState> dynamicStates{
      vk::DynamicState::eViewport,
      vk::DynamicState::eScissor,
  };

  vk::PipelineDynamicStateCreateInfo dynamicState{
      .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
      .pDynamicStates = dynamicStates.data(),
  };

  // Dynamic rendering format info
  std::vector<vk::Format> colorFormats;
  for (const auto& fmt : desc.colorFormats) {
    colorFormats.push_back(ToVkFormat(fmt));
  }

  vk::PipelineRenderingCreateInfo renderingInfo{
      .colorAttachmentCount = static_cast<uint32_t>(colorFormats.size()),
      .pColorAttachmentFormats = colorFormats.data(),
      .depthAttachmentFormat = ToVkFormat(desc.depthFormat),
  };

  vk::GraphicsPipelineCreateInfo pipelineInfo{
      .pNext = &renderingInfo,
      .stageCount = static_cast<uint32_t>(shaderStages.size()),
      .pStages = shaderStages.data(),
      .pVertexInputState = &vertexInputInfo,
      .pInputAssemblyState = &inputAssembly,
      .pViewportState = &viewportState,
      .pRasterizationState = &rasterizer,
      .pMultisampleState = &multisampling,
      .pDepthStencilState = &depthStencil,
      .pColorBlendState = &colorBlending,
      .pDynamicState = &dynamicState,
      .layout = vkLayout->GetLayout(),
  };

  auto result = context.GetDevice().createGraphicsPipelineUnique(VK_NULL_HANDLE,
                                                                 pipelineInfo);
  if (result.result != vk::Result::eSuccess) {
    return nullptr;
  }

  // Copy the layout with its push constant ranges
  std::span<const rhi::DescriptorSetLayout* const> setLayouts{
      vkLayout->GetSetLayouts().data(), vkLayout->GetSetLayouts().size()};

  auto layout = VulkanPipelineLayout::Create(context, setLayouts,
                                             vkLayout->GetPushConstantRanges());

  return std::unique_ptr<VulkanPipeline>(
      new VulkanPipeline(std::move(layout), std::move(result.value), true));
}

VulkanPipeline::VulkanPipeline(std::unique_ptr<VulkanPipelineLayout> layout,
                               vk::UniquePipeline pipeline, bool isGraphics)
    : layout_{std::move(layout)},
      pipeline_{std::move(pipeline)},
      isGraphics_{isGraphics} {}
}  // namespace backends::vulkan
