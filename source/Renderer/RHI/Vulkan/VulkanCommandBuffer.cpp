#include "Renderer/RHI/Vulkan/VulkanCommandBuffer.hpp"

#include "Core/Logger.hpp"
#include "Renderer/RHI/Vulkan/VulkanDevice.hpp"
#include "Renderer/RHI/Vulkan/VulkanSwapchain.hpp"

void RHICommandBuffer<VulkanBackend>::Allocate(const VulkanDevice& device,
                                               const vk::CommandPool& pool)
{
  vk::CommandBufferAllocateInfo alloc_info {};
  alloc_info.commandPool = pool;
  alloc_info.level = vk::CommandBufferLevel::ePrimary;
  alloc_info.commandBufferCount = 1;

  const auto command_buffers =
      device.GetVkDevice().allocateCommandBuffers(alloc_info);
  m_CommandBuffer = command_buffers.front();
}

void RHICommandBuffer<VulkanBackend>::Free(const VulkanDevice& device,
                                           const vk::CommandPool& pool)

{
  device.GetVkDevice().freeCommandBuffers(pool, 1, &m_CommandBuffer);
}

void RHICommandBuffer<VulkanBackend>::Begin()
{
  Logger::Trace("[Vulkan] Begin command buffer recording");
  vk::CommandBufferBeginInfo begin_info {};
  begin_info.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

  m_CommandBuffer.begin(begin_info);
  m_Recording = true;
}

void RHICommandBuffer<VulkanBackend>::End()
{
  if (m_InRenderPass) {
    throw std::runtime_error(
        "Trying to end command buffer without ending render pass first.");
  }

  m_CommandBuffer.end();
  m_Recording = false;
  Logger::Trace("[Vulkan] End command buffer recording");
}

void RHICommandBuffer<VulkanBackend>::BeginRenderPass(
    const VulkanSwapchain& swapchain, const RenderPassInfo& info)
{
  Logger::Trace("[Vulkan] Begin render pass ({}x{}) with dynamic rendering",
                info.Width,
                info.Height);
  m_CurrentRenderPass = info;
  m_InRenderPass = true;

  vk::ImageMemoryBarrier barrier {};
  barrier.oldLayout = vk::ImageLayout::eUndefined;
  barrier.newLayout = vk::ImageLayout::eColorAttachmentOptimal;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = swapchain.GetCurrentImage();
  barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;
  barrier.srcAccessMask = vk::AccessFlagBits::eNone;
  barrier.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;

  m_CommandBuffer.pipelineBarrier(
      vk::PipelineStageFlagBits::eTopOfPipe,
      vk::PipelineStageFlagBits::eColorAttachmentOutput,
      vk::DependencyFlags {},
      0,
      nullptr,
      0,
      nullptr,
      1,
      &barrier);

  vk::RenderingAttachmentInfo color_attachment {};
  color_attachment.imageView = swapchain.GetCurrentImageView();
  color_attachment.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;

  if (info.ColorAttachment.ColorLoadOp == LoadOp::Clear) {
    color_attachment.loadOp = vk::AttachmentLoadOp::eClear;
    const auto& clear = info.ColorAttachment.ClearColor;
    color_attachment.clearValue.color = vk::ClearColorValue(
        std::array<float, 4> {clear.R, clear.G, clear.B, clear.A});
  } else if (info.ColorAttachment.ColorLoadOp == LoadOp::Load) {
    color_attachment.loadOp = vk::AttachmentLoadOp::eLoad;
  } else {
    color_attachment.loadOp = vk::AttachmentLoadOp::eDontCare;
  }

  color_attachment.storeOp = info.ColorAttachment.ColorStoreOp == StoreOp::Store
      ? vk::AttachmentStoreOp::eStore
      : vk::AttachmentStoreOp::eDontCare;

  vk::RenderingInfo rendering_info {};
  rendering_info.renderArea.offset = vk::Offset2D {0, 0};
  rendering_info.renderArea.extent = vk::Extent2D {info.Width, info.Height};
  rendering_info.layerCount = 1;
  rendering_info.colorAttachmentCount = 1;
  rendering_info.pColorAttachments = &color_attachment;

  m_CommandBuffer.beginRendering(rendering_info);
}

void RHICommandBuffer<VulkanBackend>::EndRenderPass(
    const VulkanSwapchain& swapchain)
{
  if (!m_InRenderPass) {
    return;
  }

  Logger::Trace("[Vulkan] End render pass");
  m_CommandBuffer.endRendering();

  vk::ImageMemoryBarrier barrier {};
  barrier.oldLayout = vk::ImageLayout::eColorAttachmentOptimal;
  barrier.newLayout = vk::ImageLayout::ePresentSrcKHR;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = swapchain.GetCurrentImage();
  barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;
  barrier.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
  barrier.dstAccessMask = vk::AccessFlagBits::eNone;

  m_CommandBuffer.pipelineBarrier(
      vk::PipelineStageFlagBits::eColorAttachmentOutput,
      vk::PipelineStageFlagBits::eBottomOfPipe,
      vk::DependencyFlags {},
      0,
      nullptr,
      0,
      nullptr,
      1,
      &barrier);

  m_InRenderPass = false;
  m_CurrentRenderPass = {};
}

void RHICommandBuffer<VulkanBackend>::ClearColor(
    const VulkanSwapchain& swapchain, float r, float g, float b, float a)
{
  const vk::ClearColorValue clear_color(std::array<float, 4> {r, g, b, a});
  vk::ImageSubresourceRange range {};
  range.aspectMask = vk::ImageAspectFlagBits::eColor;
  range.baseMipLevel = 0;
  range.levelCount = 1;
  range.baseArrayLayer = 0;
  range.layerCount = 1;

  m_CommandBuffer.clearColorImage(swapchain.GetCurrentImage(),
                                  vk::ImageLayout::eTransferDstOptimal,
                                  &clear_color,
                                  1,
                                  &range);
}

auto RHICommandBuffer<VulkanBackend>::GetHandle() -> vk::CommandBuffer
{
  return m_CommandBuffer;
}
