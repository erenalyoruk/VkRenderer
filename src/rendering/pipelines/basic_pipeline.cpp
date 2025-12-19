#include "rendering/pipelines/basic_pipeline.hpp"

#include <glm/glm.hpp>

#include "vulkan/frame_data.hpp"
#include "vulkan/mesh.hpp"
#include "vulkan/pipeline.hpp"
#include "vulkan/shader.hpp"

namespace rendering {
BasicPipeline::BasicPipeline(vulkan::Context& context,
                             vulkan::Swapchain& swapchain,
                             vulkan::GlobalShaderData& globalData,
                             vulkan::MaterialPalette& materialPalette)
    : context_{context} {
  vertShader_ = vulkan::LoadShader(context_.GetDevice(),
                                   "assets/shaders/simple.vert.spv");
  fragShader_ = vulkan::LoadShader(context_.GetDevice(),
                                   "assets/shaders/simple.frag.spv");

  vulkan::DescriptorLayoutBuilder layoutBuilder{};
  layoutBuilder.AddBinding(0, vk::DescriptorType::eUniformBuffer,
                           vk::ShaderStageFlagBits::eVertex);
  layoutBuilder.AddBinding(1, vk::DescriptorType::eStorageBuffer,
                           vk::ShaderStageFlagBits::eVertex);
  descriptorSetLayout_ = layoutBuilder.Build(context_.GetDevice());

  std::vector<vulkan::DescriptorAllocator::PoolSizeRatio> poolRatios{
      {.type = vk::DescriptorType::eUniformBuffer, .ratio = 1.0F},
      {.type = vk::DescriptorType::eStorageBuffer, .ratio = 1.0F},
  };
  descriptorAllocator_ = std::make_unique<vulkan::DescriptorAllocator>(
      context_.GetDevice(), 10, poolRatios);
  descriptorSet_ = descriptorAllocator_->Allocate(descriptorSetLayout_.get());

  vulkan::DescriptorWriter writer{};
  writer.WriteBuffer(0, vk::DescriptorType::eUniformBuffer,
                     {
                         .buffer = globalData.GetBuffer().GetBuffer(),
                         .offset = 0,
                         .range = sizeof(vulkan::FrameData),
                     });

  writer.WriteBuffer(1, vk::DescriptorType::eStorageBuffer,
                     {
                         .buffer = materialPalette.GetBuffer(),
                         .offset = 0,
                         .range = materialPalette.GetMaterialCount() *
                                  sizeof(vulkan::Material),
                     });
  writer.Overwrite(context_.GetDevice(), descriptorSet_);

  vk::PushConstantRange pushConstantRange{
      .stageFlags = vk::ShaderStageFlagBits::eVertex,
      .offset = 0,
      .size = sizeof(glm::mat4),
  };
  pipelineLayout_ = context_.GetDevice().createPipelineLayoutUnique(
      vk::PipelineLayoutCreateInfo{
          .setLayoutCount = 1,
          .pSetLayouts = &descriptorSetLayout_.get(),
          .pushConstantRangeCount = 1,
          .pPushConstantRanges = &pushConstantRange,
      });

  vulkan::GraphicsPipelineBuilder pipelineBuilder{pipelineLayout_.get(),
                                                  swapchain.GetImageFormat()};
  pipelineBuilder.SetShaders(*vertShader_, *fragShader_)
      .SetDepthTest(true, true, vk::CompareOp::eLessOrEqual);
  std::vector<vk::VertexInputBindingDescription> bindings{
      {
          .binding = 0,
          .stride = sizeof(vulkan::Vertex),
          .inputRate = vk::VertexInputRate::eVertex,
      },
  };
  std::vector<vk::VertexInputAttributeDescription> attributes{
      {
          .location = 0,
          .binding = 0,
          .format = vk::Format::eR32G32B32Sfloat,
          .offset = offsetof(vulkan::Vertex, position),
      },
      {
          .location = 1,
          .binding = 0,
          .format = vk::Format::eR32G32B32Sfloat,
          .offset = offsetof(vulkan::Vertex, normal),
      },
      {
          .location = 2,
          .binding = 0,
          .format = vk::Format::eR32G32Sfloat,
          .offset = offsetof(vulkan::Vertex, texCoord),
      },
  };
  pipelineBuilder.SetVertexInput(bindings, attributes);
  pipeline_ = pipelineBuilder.Build(context_.GetDevice());
}

BasicPipeline::~BasicPipeline() = default;

void BasicPipeline::Bind(vk::CommandBuffer cmd) {
  cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline_.get());
  cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                         pipelineLayout_.get(), 0, descriptorSet_, {});
}
}  // namespace rendering
