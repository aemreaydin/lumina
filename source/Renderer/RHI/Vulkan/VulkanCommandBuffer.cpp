#include <format>

#include "Renderer/RHI/Vulkan/VulkanCommandBuffer.hpp"

#include "Core/Logger.hpp"
#include "Renderer/RHI/Vulkan/VulkanDevice.hpp"
#include "Renderer/RHI/Vulkan/VulkanSwapchain.hpp"
#include "Renderer/RHI/Vulkan/VulkanUtils.hpp"

void RHICommandBuffer<VulkanBackend>::Allocate(const VulkanDevice& device,
                                               VkCommandPool pool)
{
  VkCommandBufferAllocateInfo alloc_info = {};
  alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  alloc_info.commandPool = pool;
  alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  alloc_info.commandBufferCount = 1;

  if (auto result = VkUtils::Check(vkAllocateCommandBuffers(
          device.GetVkDevice(), &alloc_info, &m_CommandBuffer));
      !result)
  {
    throw std::runtime_error(
        std::format("Failed to allocate command buffers: {}",
                    VkUtils::ToString(result.error())));
  }
}

void RHICommandBuffer<VulkanBackend>::Free(const VulkanDevice& device,
                                           VkCommandPool pool)
{
  vkFreeCommandBuffers(device.GetVkDevice(), pool, 1, &m_CommandBuffer);
}

void RHICommandBuffer<VulkanBackend>::Begin()
{
  Logger::Trace("[Vulkan] Begin command buffer recording");
  VkCommandBufferBeginInfo begin_info = {};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  if (auto result =
          VkUtils::Check(vkBeginCommandBuffer(m_CommandBuffer, &begin_info));
      !result)
  {
    throw std::runtime_error(std::format("Failed to begin command buffer: {}",
                                         VkUtils::ToString(result.error())));
  }
  m_Recording = true;
}

void RHICommandBuffer<VulkanBackend>::End()
{
  if (m_InRenderPass) {
    throw std::runtime_error(
        "Trying to end command buffer without ending render pass first.");
  }

  if (auto result = VkUtils::Check(vkEndCommandBuffer(m_CommandBuffer));
      !result)
  {
    throw std::runtime_error(std::format("Failed to end command buffer: {}",
                                         VkUtils::ToString(result.error())));
  }
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

  VkImageMemoryBarrier barrier = {};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = swapchain.GetCurrentImage();
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;
  barrier.srcAccessMask = 0;
  barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  vkCmdPipelineBarrier(m_CommandBuffer,
                       VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                       VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                       0,
                       0,
                       nullptr,
                       0,
                       nullptr,
                       1,
                       &barrier);

  VkClearValue clear_value = {};
  if (info.ColorAttachment.ColorLoadOp == LoadOp::Clear) {
    const auto& clear = info.ColorAttachment.ClearColor;
    clear_value.color.float32[0] = clear.R;
    clear_value.color.float32[1] = clear.G;
    clear_value.color.float32[2] = clear.B;
    clear_value.color.float32[3] = clear.A;
  }

  VkAttachmentLoadOp load_op {};
  switch (info.ColorAttachment.ColorLoadOp) {
    case LoadOp::Load:
      load_op = VK_ATTACHMENT_LOAD_OP_LOAD;
      break;
    case LoadOp::Clear:
      load_op = VK_ATTACHMENT_LOAD_OP_CLEAR;
      break;
    case LoadOp::DontCare:
      load_op = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      break;
  }

  VkRenderingAttachmentInfo color_attachment = {};
  color_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
  color_attachment.imageView = swapchain.GetCurrentImageView();
  color_attachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  color_attachment.loadOp = load_op;
  color_attachment.storeOp = info.ColorAttachment.ColorStoreOp == StoreOp::Store
      ? VK_ATTACHMENT_STORE_OP_STORE
      : VK_ATTACHMENT_STORE_OP_DONT_CARE;
  color_attachment.clearValue = clear_value;

  VkRenderingInfo rendering_info = {};
  rendering_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
  rendering_info.renderArea.offset = {.x = 0, .y = 0};
  rendering_info.renderArea.extent = {.width = info.Width,
                                      .height = info.Height};
  rendering_info.layerCount = 1;
  rendering_info.colorAttachmentCount = 1;
  rendering_info.pColorAttachments = &color_attachment;

  vkCmdBeginRendering(m_CommandBuffer, &rendering_info);
}

void RHICommandBuffer<VulkanBackend>::EndRenderPass(
    const VulkanSwapchain& swapchain)
{
  if (!m_InRenderPass) {
    return;
  }

  Logger::Trace("[Vulkan] End render pass");
  vkCmdEndRendering(m_CommandBuffer);

  VkImageMemoryBarrier barrier = {};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = swapchain.GetCurrentImage();
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;
  barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  barrier.dstAccessMask = 0;

  vkCmdPipelineBarrier(m_CommandBuffer,
                       VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                       VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                       0,
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
  VkClearColorValue clear_color = {};
  clear_color.float32[0] = r;
  clear_color.float32[1] = g;
  clear_color.float32[2] = b;
  clear_color.float32[3] = a;

  VkImageSubresourceRange range = {};
  range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  range.baseMipLevel = 0;
  range.levelCount = 1;
  range.baseArrayLayer = 0;
  range.layerCount = 1;

  vkCmdClearColorImage(m_CommandBuffer,
                       swapchain.GetCurrentImage(),
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                       &clear_color,
                       1,
                       &range);
}

auto RHICommandBuffer<VulkanBackend>::GetHandle() -> VkCommandBuffer
{
  return m_CommandBuffer;
}
