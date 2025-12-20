#include "backends/vulkan/vulkan_command.hpp"

#include <bit>
#include <memory>
#include <utility>

#include "backends/vulkan/vulkan_buffer.hpp"
#include "backends/vulkan/vulkan_context.hpp"
#include "backends/vulkan/vulkan_descriptor.hpp"
#include "backends/vulkan/vulkan_pipeline.hpp"
#include "backends/vulkan/vulkan_texture.hpp"

namespace backends::vulkan {

namespace {
vk::ImageLayout ToVkLayout(rhi::ImageLayout layout) {
  switch (layout) {
    case rhi::ImageLayout::Undefined:
      return vk::ImageLayout::eUndefined;
    case rhi::ImageLayout::General:
      return vk::ImageLayout::eGeneral;
    case rhi::ImageLayout::ColorAttachment:
      return vk::ImageLayout::eColorAttachmentOptimal;
    case rhi::ImageLayout::DepthStencilAttachment:
      return vk::ImageLayout::eDepthStencilAttachmentOptimal;
    case rhi::ImageLayout::ShaderReadOnly:
      return vk::ImageLayout::eShaderReadOnlyOptimal;
    case rhi::ImageLayout::TransferSrc:
      return vk::ImageLayout::eTransferSrcOptimal;
    case rhi::ImageLayout::TransferDst:
      return vk::ImageLayout::eTransferDstOptimal;
    case rhi::ImageLayout::Present:
      return vk::ImageLayout::ePresentSrcKHR;
    default:
      return vk::ImageLayout::eUndefined;
  }
}

std::pair<vk::PipelineStageFlags, vk::AccessFlags> GetLayoutStageAndAccess(
    rhi::ImageLayout layout, bool isSrc) {
  switch (layout) {
    case rhi::ImageLayout::Undefined:
      return {vk::PipelineStageFlagBits::eTopOfPipe, vk::AccessFlags{}};
    case rhi::ImageLayout::ColorAttachment:
      return {vk::PipelineStageFlagBits::eColorAttachmentOutput,
              isSrc ? vk::AccessFlagBits::eColorAttachmentWrite
                    : vk::AccessFlagBits::eColorAttachmentRead |
                          vk::AccessFlagBits::eColorAttachmentWrite};
    case rhi::ImageLayout::DepthStencilAttachment:
      return {vk::PipelineStageFlagBits::eEarlyFragmentTests |
                  vk::PipelineStageFlagBits::eLateFragmentTests,
              vk::AccessFlagBits::eDepthStencilAttachmentRead |
                  vk::AccessFlagBits::eDepthStencilAttachmentWrite};
    case rhi::ImageLayout::ShaderReadOnly:
      return {vk::PipelineStageFlagBits::eFragmentShader,
              vk::AccessFlagBits::eShaderRead};
    case rhi::ImageLayout::TransferSrc:
      return {vk::PipelineStageFlagBits::eTransfer,
              vk::AccessFlagBits::eTransferRead};
    case rhi::ImageLayout::TransferDst:
      return {vk::PipelineStageFlagBits::eTransfer,
              vk::AccessFlagBits::eTransferWrite};
    case rhi::ImageLayout::Present:
      return {vk::PipelineStageFlagBits::eBottomOfPipe, vk::AccessFlags{}};
    default:
      return {
          vk::PipelineStageFlagBits::eAllCommands,
          vk::AccessFlagBits::eMemoryRead | vk::AccessFlagBits::eMemoryWrite};
  }
}
}  // namespace

VulkanCommandBuffer::VulkanCommandBuffer(vk::CommandBuffer commandBuffer,
                                         vk::Device device)
    : commandBuffer_{commandBuffer}, device_{device} {}

void VulkanCommandBuffer::Begin() {
  vk::CommandBufferBeginInfo beginInfo{
      .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
  };
  commandBuffer_.begin(beginInfo);
}

void VulkanCommandBuffer::End() { commandBuffer_.end(); }

void VulkanCommandBuffer::BeginRendering(const rhi::RenderingInfo& info) {
  std::vector<vk::RenderingAttachmentInfo> colorAttachments;
  for (const auto& attachment : info.colorAttachments) {
    auto* vkTexture = std::bit_cast<VulkanTexture*>(attachment.texture);

    vk::AttachmentLoadOp loadOp = vk::AttachmentLoadOp::eDontCare;
    switch (attachment.loadOp) {
      case rhi::LoadOp::Load:
        loadOp = vk::AttachmentLoadOp::eLoad;
        break;
      case rhi::LoadOp::Clear:
        loadOp = vk::AttachmentLoadOp::eClear;
        break;
      case rhi::LoadOp::DontCare:
        loadOp = vk::AttachmentLoadOp::eDontCare;
        break;
    }

    vk::AttachmentStoreOp storeOp = vk::AttachmentStoreOp::eStore;
    switch (attachment.storeOp) {
      case rhi::StoreOp::Store:
        storeOp = vk::AttachmentStoreOp::eStore;
        break;
      case rhi::StoreOp::DontCare:
        storeOp = vk::AttachmentStoreOp::eDontCare;
        break;
    }

    vk::RenderingAttachmentInfo colorAttachment{
        .imageView = vkTexture->GetImageView(),
        .imageLayout = ToVkLayout(attachment.layout),
        .loadOp = loadOp,
        .storeOp = storeOp,
        .clearValue =
            vk::ClearValue{
                .color = vk::ClearColorValue{std::array{
                    attachment.clearValue[0], attachment.clearValue[1],
                    attachment.clearValue[2], attachment.clearValue[3]}}},
    };
    colorAttachments.push_back(colorAttachment);
  }

  vk::RenderingAttachmentInfo depthAttachment{};
  if (info.depthAttachment != nullptr) {
    auto* vkTexture =
        std::bit_cast<VulkanTexture*>(info.depthAttachment->texture);

    vk::AttachmentLoadOp loadOp = vk::AttachmentLoadOp::eDontCare;
    switch (info.depthAttachment->loadOp) {
      case rhi::LoadOp::Load:
        loadOp = vk::AttachmentLoadOp::eLoad;
        break;
      case rhi::LoadOp::Clear:
        loadOp = vk::AttachmentLoadOp::eClear;
        break;
      case rhi::LoadOp::DontCare:
        loadOp = vk::AttachmentLoadOp::eDontCare;
        break;
    }

    vk::AttachmentStoreOp storeOp = vk::AttachmentStoreOp::eStore;
    switch (info.depthAttachment->storeOp) {
      case rhi::StoreOp::Store:
        storeOp = vk::AttachmentStoreOp::eStore;
        break;
      case rhi::StoreOp::DontCare:
        storeOp = vk::AttachmentStoreOp::eDontCare;
        break;
    }

    depthAttachment = {
        .imageView = vkTexture->GetImageView(),
        .imageLayout = ToVkLayout(info.depthAttachment->layout),
        .loadOp = loadOp,
        .storeOp = storeOp,
        .clearValue =
            vk::ClearValue{.depthStencil = {.depth = 1.0F, .stencil = 0}},
    };
  }

  vk::RenderingInfo renderingInfo{
      .renderArea = {.offset = {.x = 0, .y = 0},
                     .extent = {.width = info.width, .height = info.height}},
      .layerCount = 1,
      .colorAttachmentCount = static_cast<uint32_t>(colorAttachments.size()),
      .pColorAttachments = colorAttachments.data(),
      .pDepthAttachment =
          (info.depthAttachment != nullptr) ? &depthAttachment : nullptr,
  };

  commandBuffer_.beginRendering(renderingInfo);
}

void VulkanCommandBuffer::EndRendering() { commandBuffer_.endRendering(); }

void VulkanCommandBuffer::SetViewport(float x, float y, float width,
                                      float height, float minDepth,
                                      float maxDepth) {
  vk::Viewport viewport{
      .x = x,
      .y = y,
      .width = width,
      .height = height,
      .minDepth = minDepth,
      .maxDepth = maxDepth,
  };
  commandBuffer_.setViewport(0, viewport);
}

void VulkanCommandBuffer::SetScissor(int32_t x, int32_t y, uint32_t width,
                                     uint32_t height) {
  vk::Rect2D scissor{
      .offset = {.x = x, .y = y},
      .extent = {.width = width, .height = height},
  };
  commandBuffer_.setScissor(0, scissor);
}

void VulkanCommandBuffer::BindPipeline(const rhi::Pipeline* pipeline) {
  const auto* vkPipeline = std::bit_cast<const VulkanPipeline*>(pipeline);
  vk::PipelineBindPoint bindPoint = vkPipeline->IsGraphics()
                                        ? vk::PipelineBindPoint::eGraphics
                                        : vk::PipelineBindPoint::eCompute;
  commandBuffer_.bindPipeline(bindPoint, vkPipeline->GetPipeline());
}

void VulkanCommandBuffer::BindDescriptorSets(
    const rhi::Pipeline* pipeline, uint32_t firstSet,
    std::span<const rhi::DescriptorSet* const> sets) {
  std::vector<vk::DescriptorSet> vkSets;
  for (const auto* set : sets) {
    const auto* vkSet = std::bit_cast<const VulkanDescriptorSet*>(set);
    vkSets.push_back(vkSet->GetSet());
  }
  const auto* vkPipeline = std::bit_cast<const VulkanPipeline*>(pipeline);
  const auto* vkPipelineLayout =
      std::bit_cast<const VulkanPipelineLayout*>(&vkPipeline->GetLayout());
  vk::PipelineBindPoint bindPoint = vkPipeline->IsGraphics()
                                        ? vk::PipelineBindPoint::eGraphics
                                        : vk::PipelineBindPoint::eCompute;
  commandBuffer_.bindDescriptorSets(bindPoint, vkPipelineLayout->GetLayout(),
                                    firstSet, vkSets, {});
}

void VulkanCommandBuffer::BindVertexBuffers(
    uint32_t firstBinding, std::span<const rhi::Buffer* const> buffers,
    std::span<const uint64_t> offsets) {
  std::vector<vk::Buffer> vkBuffers;
  std::vector<vk::DeviceSize> vkOffsets;
  for (const auto* buffer : buffers) {
    const auto* vkBuffer = std::bit_cast<const VulkanBuffer*>(buffer);
    vkBuffers.push_back(vkBuffer->GetHandle());
  }
  vkOffsets.assign(offsets.begin(), offsets.end());
  commandBuffer_.bindVertexBuffers(firstBinding, vkBuffers, vkOffsets);
}

void VulkanCommandBuffer::BindIndexBuffer(const rhi::Buffer& buffer,
                                          uint64_t offset, bool is32Bit) {
  const auto* vkBuffer = std::bit_cast<const VulkanBuffer*>(&buffer);
  vk::IndexType indexType =
      is32Bit ? vk::IndexType::eUint32 : vk::IndexType::eUint16;
  commandBuffer_.bindIndexBuffer(vkBuffer->GetHandle(), offset, indexType);
}

void VulkanCommandBuffer::Draw(uint32_t vertexCount, uint32_t instanceCount,
                               uint32_t firstVertex, uint32_t firstInstance) {
  commandBuffer_.draw(vertexCount, instanceCount, firstVertex, firstInstance);
}

void VulkanCommandBuffer::DrawIndexed(uint32_t indexCount,
                                      uint32_t instanceCount,
                                      uint32_t firstIndex, int32_t vertexOffset,
                                      uint32_t firstInstance) {
  commandBuffer_.drawIndexed(indexCount, instanceCount, firstIndex,
                             vertexOffset, firstInstance);
}

void VulkanCommandBuffer::DrawIndexedIndirect(const rhi::Buffer* buffer,
                                              rhi::Size offset,
                                              uint32_t drawCount,
                                              uint32_t stride) {
  const auto* vkBuffer = std::bit_cast<const VulkanBuffer*>(buffer);
  commandBuffer_.drawIndexedIndirect(vkBuffer->GetHandle(), offset, drawCount,
                                     stride);
}

void VulkanCommandBuffer::DrawIndexedIndirectCount(
    const rhi::Buffer* commandBuffer, rhi::Size commandOffset,
    const rhi::Buffer* countBuffer, rhi::Size countOffset,
    uint32_t maxDrawCount, uint32_t stride) {
  const auto* vkCmdBuffer = std::bit_cast<const VulkanBuffer*>(commandBuffer);
  const auto* vkCountBuffer = std::bit_cast<const VulkanBuffer*>(countBuffer);

  commandBuffer_.drawIndexedIndirectCount(
      vkCmdBuffer->GetHandle(), commandOffset, vkCountBuffer->GetHandle(),
      countOffset, maxDrawCount, stride);
}

void VulkanCommandBuffer::Dispatch(uint32_t groupCountX, uint32_t groupCountY,
                                   uint32_t groupCountZ) {
  commandBuffer_.dispatch(groupCountX, groupCountY, groupCountZ);
}

void VulkanCommandBuffer::BufferBarrier(const rhi::Buffer* buffer,
                                        rhi::AccessFlags srcAccess,
                                        rhi::AccessFlags dstAccess) {
  const auto* vkBuffer = std::bit_cast<const VulkanBuffer*>(buffer);

  auto convertAccess = [](rhi::AccessFlags flags) -> vk::AccessFlags2 {
    vk::AccessFlags2 result{};
    if ((flags & rhi::AccessFlags::ShaderRead) != rhi::AccessFlags::None) {
      result |= vk::AccessFlagBits2::eShaderRead;
    }
    if ((flags & rhi::AccessFlags::ShaderWrite) != rhi::AccessFlags::None) {
      result |= vk::AccessFlagBits2::eShaderWrite;
    }
    if ((flags & rhi::AccessFlags::IndirectCommandRead) !=
        rhi::AccessFlags::None) {
      result |= vk::AccessFlagBits2::eIndirectCommandRead;
    }
    if ((flags & rhi::AccessFlags::TransferRead) != rhi::AccessFlags::None) {
      result |= vk::AccessFlagBits2::eTransferRead;
    }
    if ((flags & rhi::AccessFlags::TransferWrite) != rhi::AccessFlags::None) {
      result |= vk::AccessFlagBits2::eTransferWrite;
    }
    return result;
  };

  // Determine source stage based on access flags
  auto getSrcStage = [](rhi::AccessFlags flags) -> vk::PipelineStageFlags2 {
    vk::PipelineStageFlags2 stage{};
    if ((flags & rhi::AccessFlags::TransferWrite) != rhi::AccessFlags::None ||
        (flags & rhi::AccessFlags::TransferRead) != rhi::AccessFlags::None) {
      stage |= vk::PipelineStageFlagBits2::eTransfer;
    }
    if ((flags & rhi::AccessFlags::ShaderRead) != rhi::AccessFlags::None ||
        (flags & rhi::AccessFlags::ShaderWrite) != rhi::AccessFlags::None) {
      stage |= vk::PipelineStageFlagBits2::eComputeShader;
    }
    if (stage == vk::PipelineStageFlags2{}) {
      stage = vk::PipelineStageFlagBits2::eAllCommands;
    }
    return stage;
  };

  // Determine destination stage based on access flags
  auto getDstStage = [](rhi::AccessFlags flags) -> vk::PipelineStageFlags2 {
    vk::PipelineStageFlags2 stage{};
    if ((flags & rhi::AccessFlags::IndirectCommandRead) !=
        rhi::AccessFlags::None) {
      stage |= vk::PipelineStageFlagBits2::eDrawIndirect;
    }
    if ((flags & rhi::AccessFlags::ShaderRead) != rhi::AccessFlags::None ||
        (flags & rhi::AccessFlags::ShaderWrite) != rhi::AccessFlags::None) {
      stage |= vk::PipelineStageFlagBits2::eVertexShader |
               vk::PipelineStageFlagBits2::eComputeShader;
    }
    if ((flags & rhi::AccessFlags::TransferRead) != rhi::AccessFlags::None ||
        (flags & rhi::AccessFlags::TransferWrite) != rhi::AccessFlags::None) {
      stage |= vk::PipelineStageFlagBits2::eTransfer;
    }
    if (stage == vk::PipelineStageFlags2{}) {
      stage = vk::PipelineStageFlagBits2::eAllCommands;
    }
    return stage;
  };

  vk::BufferMemoryBarrier2 barrier{
      .srcStageMask = getSrcStage(srcAccess),
      .srcAccessMask = convertAccess(srcAccess),
      .dstStageMask = getDstStage(dstAccess),
      .dstAccessMask = convertAccess(dstAccess),
      .buffer = vkBuffer->GetHandle(),
      .offset = 0,
      .size = VK_WHOLE_SIZE,
  };

  vk::DependencyInfo depInfo{
      .bufferMemoryBarrierCount = 1,
      .pBufferMemoryBarriers = &barrier,
  };

  commandBuffer_.pipelineBarrier2(depInfo);
}

void VulkanCommandBuffer::FillBuffer(rhi::Buffer* buffer, rhi::Size offset,
                                     rhi::Size size, uint32_t value) {
  auto* vkBuffer = std::bit_cast<VulkanBuffer*>(buffer);
  commandBuffer_.fillBuffer(vkBuffer->GetHandle(), offset, size, value);
}

void VulkanCommandBuffer::TransitionTexture(rhi::Texture* texture,
                                            rhi::ImageLayout oldLayout,
                                            rhi::ImageLayout newLayout) {
  auto* vkTexture = std::bit_cast<VulkanTexture*>(texture);

  auto [srcStage, srcAccess] = GetLayoutStageAndAccess(oldLayout, true);
  auto [dstStage, dstAccess] = GetLayoutStageAndAccess(newLayout, false);

  bool isDepth = vkTexture->GetFormat() == rhi::Format::D32Sfloat ||
                 vkTexture->GetFormat() == rhi::Format::D16Unorm ||
                 vkTexture->GetFormat() == rhi::Format::D24UnormS8Uint ||
                 vkTexture->GetFormat() == rhi::Format::D32SfloatS8Uint;

  vk::ImageMemoryBarrier barrier{
      .srcAccessMask = srcAccess,
      .dstAccessMask = dstAccess,
      .oldLayout = ToVkLayout(oldLayout),
      .newLayout = ToVkLayout(newLayout),
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = vkTexture->GetImage(),
      .subresourceRange =
          {
              .aspectMask = isDepth ? vk::ImageAspectFlagBits::eDepth
                                    : vk::ImageAspectFlagBits::eColor,
              .baseMipLevel = 0,
              .levelCount = vkTexture->GetMipLevels(),
              .baseArrayLayer = 0,
              .layerCount = 1,
          },
  };

  commandBuffer_.pipelineBarrier(srcStage, dstStage, {}, {}, {}, barrier);
}

void VulkanCommandBuffer::CopyBuffer(const rhi::Buffer* src, rhi::Buffer* dst,
                                     rhi::Size srcOffset, rhi::Size dstOffset,
                                     rhi::Size size) {
  const auto* vkSrc = std::bit_cast<const VulkanBuffer*>(src);
  auto* vkDst = std::bit_cast<VulkanBuffer*>(dst);

  vk::BufferCopy copyRegion{
      .srcOffset = srcOffset,
      .dstOffset = dstOffset,
      .size = size,
  };

  commandBuffer_.copyBuffer(vkSrc->GetHandle(), vkDst->GetHandle(), copyRegion);
}

void VulkanCommandBuffer::CopyBufferToTexture(const rhi::Buffer* src,
                                              rhi::Texture* dst,
                                              uint32_t mipLevel,
                                              uint32_t arrayLayer) {
  const auto* vkSrc = std::bit_cast<const VulkanBuffer*>(src);
  auto* vkDst = std::bit_cast<VulkanTexture*>(dst);

  vk::BufferImageCopy copyRegion{
      .bufferOffset = 0,
      .bufferRowLength = 0,
      .bufferImageHeight = 0,
      .imageSubresource =
          {
              .aspectMask = vk::ImageAspectFlagBits::eColor,
              .mipLevel = mipLevel,
              .baseArrayLayer = arrayLayer,
              .layerCount = 1,
          },
      .imageOffset = {.x = 0, .y = 0, .z = 0},
      .imageExtent = {.width = vkDst->GetWidth(),
                      .height = vkDst->GetHeight(),
                      .depth = 1},
  };

  commandBuffer_.copyBufferToImage(vkSrc->GetHandle(), vkDst->GetImage(),
                                   vk::ImageLayout::eTransferDstOptimal,
                                   copyRegion);
}

void VulkanCommandBuffer::PushConstants(const rhi::Pipeline* pipeline,
                                        uint32_t offset,
                                        std::span<const std::byte> data) {
  const auto* vkPipeline = std::bit_cast<const VulkanPipeline*>(pipeline);
  const auto* vkPipelineLayout =
      std::bit_cast<const VulkanPipelineLayout*>(&vkPipeline->GetLayout());

  // Get shader stages from push constant ranges
  // For simplicity, use all stages that are commonly used
  vk::ShaderStageFlags stageFlags =
      vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment;

  // If layout has push constant ranges, use the appropriate stages
  const auto& ranges = vkPipelineLayout->GetPushConstantRanges();
  if (!ranges.empty()) {
    stageFlags = {};
    for (const auto& range : ranges) {
      // Check if this range covers the offset
      if (offset >= range.offset && offset < range.offset + range.size) {
        switch (range.stage) {
          case rhi::ShaderStage::Vertex:
            stageFlags |= vk::ShaderStageFlagBits::eVertex;
            break;
          case rhi::ShaderStage::Fragment:
            stageFlags |= vk::ShaderStageFlagBits::eFragment;
            break;
          case rhi::ShaderStage::Compute:
            stageFlags |= vk::ShaderStageFlagBits::eCompute;
            break;
        }
      }
    }
    // If no matching range found, default to vertex + fragment
    if (!stageFlags) {
      stageFlags =
          vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment;
    }
  }

  commandBuffer_.pushConstants(vkPipelineLayout->GetLayout(), stageFlags,
                               offset, static_cast<uint32_t>(data.size()),
                               data.data());
}

VulkanCommandPool::VulkanCommandPool(vk::UniqueCommandPool commandPool,
                                     vk::Device device)
    : commandPool_{std::move(commandPool)}, device_{device} {}

std::unique_ptr<VulkanCommandPool> VulkanCommandPool::Create(
    VulkanContext& context, rhi::QueueType queueType) {
  uint32_t familyIndex = 0;
  if (queueType == rhi::QueueType::Compute) {
    familyIndex = context.GetComputeFamilyIndex();
  } else if (queueType == rhi::QueueType::Transfer) {
    familyIndex = context.GetTransferFamilyIndex();
  } else {
    familyIndex = context.GetGraphicsFamilyIndex();
  }

  vk::CommandPoolCreateInfo poolInfo{
      .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
      .queueFamilyIndex = familyIndex,
  };

  vk::UniqueCommandPool commandPool =
      context.GetDevice().createCommandPoolUnique(poolInfo);

  return std::unique_ptr<VulkanCommandPool>(
      new VulkanCommandPool(std::move(commandPool), context.GetDevice()));
}

void VulkanCommandPool::Reset() {
  device_.resetCommandPool(commandPool_.get());
  // DO NOT CLEAR allocatedBuffers_ HERE! It would invalidate existing command
  // buffers.
  // allocatedBuffers_.clear();
}

rhi::CommandBuffer* VulkanCommandPool::AllocateCommandBuffer() {
  vk::CommandBufferAllocateInfo allocInfo{
      .commandPool = commandPool_.get(),
      .level = vk::CommandBufferLevel::ePrimary,
      .commandBufferCount = 1,
  };

  vk::CommandBuffer commandBuffer =
      device_.allocateCommandBuffers(allocInfo)[0];

  auto buffer = std::make_unique<VulkanCommandBuffer>(commandBuffer, device_);
  rhi::CommandBuffer* ptr = buffer.get();
  allocatedBuffers_.push_back(std::move(buffer));
  return ptr;
}
}  // namespace backends::vulkan
