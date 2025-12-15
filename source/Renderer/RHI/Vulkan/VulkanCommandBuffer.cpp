#include <format>

#include "Renderer/RHI/Vulkan/VulkanCommandBuffer.hpp"

#include <vulkan/vulkan_core.h>

#include "Core/Logger.hpp"
#include "Renderer/RHI/Vulkan/VulkanBuffer.hpp"
#include "Renderer/RHI/Vulkan/VulkanDevice.hpp"
#include "Renderer/RHI/Vulkan/VulkanShaderModule.hpp"
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

void RHICommandBuffer<VulkanBackend>::BindShaders(
    const VulkanShaderModule* vertex_shader,
    const VulkanShaderModule* fragment_shader)
{
  std::array<VkShaderStageFlagBits, 2> stages = {VK_SHADER_STAGE_VERTEX_BIT,
                                                 VK_SHADER_STAGE_FRAGMENT_BIT};
  std::array<VkShaderEXT, 2> shaders = {
      vertex_shader != nullptr ? vertex_shader->GetVkShaderEXT()
                               : VK_NULL_HANDLE,
      fragment_shader != nullptr ? fragment_shader->GetVkShaderEXT()
                                 : VK_NULL_HANDLE};

  vkCmdBindShadersEXT(m_CommandBuffer, 2, stages.data(), shaders.data());

  const auto sample_mask = 0xFFFFFFFF;
  vkCmdSetSampleMaskEXT(m_CommandBuffer, VK_SAMPLE_COUNT_1_BIT, &sample_mask);
  vkCmdSetRasterizationSamplesEXT(m_CommandBuffer, VK_SAMPLE_COUNT_1_BIT);
  vkCmdSetRasterizerDiscardEnable(m_CommandBuffer, VK_FALSE);
  vkCmdSetAlphaToCoverageEnableEXT(m_CommandBuffer, VK_FALSE);
  vkCmdSetPolygonModeEXT(m_CommandBuffer, VK_POLYGON_MODE_FILL);
  vkCmdSetCullMode(m_CommandBuffer, VK_CULL_MODE_NONE);
  vkCmdSetFrontFace(m_CommandBuffer, VK_FRONT_FACE_COUNTER_CLOCKWISE);
  vkCmdSetDepthTestEnable(m_CommandBuffer, VK_FALSE);
  vkCmdSetDepthWriteEnable(m_CommandBuffer, VK_FALSE);
  vkCmdSetDepthBiasEnable(m_CommandBuffer, VK_FALSE);
  vkCmdSetStencilTestEnable(m_CommandBuffer, VK_FALSE);
  vkCmdSetPrimitiveRestartEnable(m_CommandBuffer, VK_FALSE);

  const VkViewport viewport {
      .x = 0,
      .y = static_cast<float>(m_CurrentRenderPass.Height),
      .width = static_cast<float>(m_CurrentRenderPass.Width),
      .height = -static_cast<float>(m_CurrentRenderPass.Height),
      .minDepth = 0.0F,
      .maxDepth = 1.0F,
  };
  vkCmdSetViewportWithCount(m_CommandBuffer, 1, &viewport);

  const VkRect2D scissor {
      .offset = VkOffset2D {.x = 0, .y = 0},
      .extent = VkExtent2D {.width = m_CurrentRenderPass.Width,
                            .height = m_CurrentRenderPass.Height},
  };
  vkCmdSetScissorWithCount(m_CommandBuffer, 1, &scissor);

  // Color blend state
  VkColorBlendEquationEXT color_blend_equation {};
  color_blend_equation.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
  color_blend_equation.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
  color_blend_equation.colorBlendOp = VK_BLEND_OP_ADD;
  color_blend_equation.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  color_blend_equation.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  color_blend_equation.alphaBlendOp = VK_BLEND_OP_ADD;
  vkCmdSetColorBlendEquationEXT(m_CommandBuffer, 0, 1, &color_blend_equation);

  const VkBool32 color_blend_enable = VK_FALSE;
  vkCmdSetColorBlendEnableEXT(m_CommandBuffer, 0, 1, &color_blend_enable);

  const VkColorComponentFlags color_write_mask = VK_COLOR_COMPONENT_R_BIT
      | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT
      | VK_COLOR_COMPONENT_A_BIT;
  vkCmdSetColorWriteMaskEXT(m_CommandBuffer, 0, 1, &color_write_mask);

  Logger::Trace("[Vulkan] Bound shaders");
}

void RHICommandBuffer<VulkanBackend>::BindVertexBuffer(
    const VulkanBuffer& buffer, uint32_t binding)
{
  VkBuffer vk_buffer = buffer.GetVkBuffer();
  const VkDeviceSize offset = 0;
  vkCmdBindVertexBuffers(m_CommandBuffer, binding, 1, &vk_buffer, &offset);
}

void RHICommandBuffer<VulkanBackend>::SetVertexInput(
    const VertexInputLayout& layout)
{
  std::vector<VkVertexInputBindingDescription2EXT> bindings;
  std::vector<VkVertexInputAttributeDescription2EXT> attributes;

  VkVertexInputBindingDescription2EXT binding {};
  binding.sType = VK_STRUCTURE_TYPE_VERTEX_INPUT_BINDING_DESCRIPTION_2_EXT;
  binding.binding = 0;
  binding.stride = layout.Stride;
  binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
  binding.divisor = 1;
  bindings.push_back(binding);

  for (const auto& attr : layout.Attributes) {
    VkVertexInputAttributeDescription2EXT vk_attr {};
    vk_attr.sType = VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT;
    vk_attr.location = attr.Location;
    vk_attr.binding = 0;
    vk_attr.offset = attr.Offset;

    switch (attr.Format) {
      case VertexFormat::Float:
        vk_attr.format = VK_FORMAT_R32_SFLOAT;
        break;
      case VertexFormat::Float2:
        vk_attr.format = VK_FORMAT_R32G32_SFLOAT;
        break;
      case VertexFormat::Float3:
        vk_attr.format = VK_FORMAT_R32G32B32_SFLOAT;
        break;
      case VertexFormat::Float4:
        vk_attr.format = VK_FORMAT_R32G32B32A32_SFLOAT;
        break;
      default:
        vk_attr.format = VK_FORMAT_R32G32B32_SFLOAT;
        break;
    }

    attributes.push_back(vk_attr);
  }

  vkCmdSetVertexInputEXT(m_CommandBuffer,
                         static_cast<uint32_t>(bindings.size()),
                         bindings.data(),
                         static_cast<uint32_t>(attributes.size()),
                         attributes.data());
}

void RHICommandBuffer<VulkanBackend>::SetPrimitiveTopology(
    PrimitiveTopology topology)
{
  VkPrimitiveTopology vk_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  switch (topology) {
    case PrimitiveTopology::TriangleList:
      vk_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
      break;
    case PrimitiveTopology::TriangleStrip:
      vk_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
      break;
    case PrimitiveTopology::LineList:
      vk_topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
      break;
    case PrimitiveTopology::LineStrip:
      vk_topology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
      break;
    case PrimitiveTopology::PointList:
      vk_topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
      break;
  }
  vkCmdSetPrimitiveTopology(m_CommandBuffer, vk_topology);
}

void RHICommandBuffer<VulkanBackend>::Draw(uint32_t vertex_count,
                                           uint32_t instance_count,
                                           uint32_t first_vertex,
                                           uint32_t first_instance)
{
  vkCmdDraw(m_CommandBuffer,
            vertex_count,
            instance_count,
            first_vertex,
            first_instance);
}
