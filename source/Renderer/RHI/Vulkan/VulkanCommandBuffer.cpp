#include <format>

#include "Renderer/RHI/Vulkan/VulkanCommandBuffer.hpp"

#include <vulkan/vulkan_core.h>

#include "Core/Logger.hpp"
#include "Renderer/RHI/Vulkan/VulkanBuffer.hpp"
#include "Renderer/RHI/Vulkan/VulkanDescriptorSet.hpp"
#include "Renderer/RHI/Vulkan/VulkanDevice.hpp"
#include "Renderer/RHI/Vulkan/VulkanPipelineLayout.hpp"
#include "Renderer/RHI/Vulkan/VulkanRenderTarget.hpp"
#include "Renderer/RHI/Vulkan/VulkanShaderModule.hpp"
#include "Renderer/RHI/Vulkan/VulkanSwapchain.hpp"
#include "Renderer/RHI/Vulkan/VulkanUtils.hpp"

void VulkanCommandBuffer::Allocate(const VulkanDevice& device,
                                   VkCommandPool pool)
{
  m_Device = &device;

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

void VulkanCommandBuffer::Free(const VulkanDevice& device, VkCommandPool pool)
{
  vkFreeCommandBuffers(device.GetVkDevice(), pool, 1, &m_CommandBuffer);
}

void VulkanCommandBuffer::Begin()
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

void VulkanCommandBuffer::End()
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

static auto ToVkLoadOp(LoadOp op) -> VkAttachmentLoadOp
{
  switch (op) {
    case LoadOp::Load:
      return VK_ATTACHMENT_LOAD_OP_LOAD;
    case LoadOp::Clear:
      return VK_ATTACHMENT_LOAD_OP_CLEAR;
    case LoadOp::DontCare:
      return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  }
  return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
}

void VulkanCommandBuffer::BeginRenderPass(const RenderPassInfo& info)
{
  Logger::Trace("[Vulkan] Begin render pass ({}x{}) with dynamic rendering",
                info.Width,
                info.Height);
  m_CurrentRenderPass = info;
  m_InRenderPass = true;

  // Resolve color/depth images from either swapchain or render target
  VkImage color_image = VK_NULL_HANDLE;
  VkImageView color_view = VK_NULL_HANDLE;
  VkImage depth_image = VK_NULL_HANDLE;
  VkImageView depth_view = VK_NULL_HANDLE;

  if (info.RenderTarget == nullptr) {
    // Swapchain path
    m_IsSwapchainTarget = true;
    auto* swapchain = dynamic_cast<VulkanSwapchain*>(m_Device->GetSwapchain());
    color_image = swapchain->GetCurrentImage();
    color_view = swapchain->GetCurrentImageView();
    if (info.DepthStencilAttachment != nullptr) {
      depth_image = swapchain->GetDepthImage();
      depth_view = swapchain->GetDepthImageView();
    }
  } else {
    // Off-screen render target path
    m_IsSwapchainTarget = false;
    auto* rt = dynamic_cast<VulkanRenderTarget*>(info.RenderTarget);
    color_image = rt->GetColorImage();
    color_view = rt->GetColorImageView();
    if (info.DepthStencilAttachment != nullptr) {
      depth_image = rt->GetDepthImage();
      depth_view = rt->GetDepthImageView();
    }
  }

  // Barriers
  std::vector<VkImageMemoryBarrier> barriers;
  VkPipelineStageFlags dst_stage =
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

  VkImageMemoryBarrier color_barrier {};
  color_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  color_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  color_barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  color_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  color_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  color_barrier.image = color_image;
  color_barrier.subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                    .baseMipLevel = 0,
                                    .levelCount = 1,
                                    .baseArrayLayer = 0,
                                    .layerCount = 1};
  color_barrier.srcAccessMask = 0;
  color_barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  barriers.push_back(color_barrier);

  if (info.DepthStencilAttachment != nullptr && depth_image != VK_NULL_HANDLE) {
    VkImageMemoryBarrier depth_barrier {};
    depth_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    depth_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depth_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    depth_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    depth_barrier.image = depth_image;
    depth_barrier.subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
                                      .baseMipLevel = 0,
                                      .levelCount = 1,
                                      .baseArrayLayer = 0,
                                      .layerCount = 1};
    depth_barrier.srcAccessMask = 0;
    depth_barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    barriers.push_back(depth_barrier);
    dst_stage |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  }

  vkCmdPipelineBarrier(m_CommandBuffer,
                       VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                       dst_stage,
                       0,
                       0,
                       nullptr,
                       0,
                       nullptr,
                       static_cast<uint32_t>(barriers.size()),
                       barriers.data());

  // Color attachment
  VkClearValue clear_value {};
  if (info.ColorAttachment.ColorLoadOp == LoadOp::Clear) {
    const auto& c = info.ColorAttachment.ClearColor;
    clear_value.color.float32[0] = c.R;
    clear_value.color.float32[1] = c.G;
    clear_value.color.float32[2] = c.B;
    clear_value.color.float32[3] = c.A;
  }

  VkRenderingAttachmentInfo color_attachment {};
  color_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
  color_attachment.imageView = color_view;
  color_attachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  color_attachment.loadOp = ToVkLoadOp(info.ColorAttachment.ColorLoadOp);
  color_attachment.storeOp = info.ColorAttachment.ColorStoreOp == StoreOp::Store
      ? VK_ATTACHMENT_STORE_OP_STORE
      : VK_ATTACHMENT_STORE_OP_DONT_CARE;
  color_attachment.clearValue = clear_value;

  // Depth attachment
  VkRenderingAttachmentInfo depth_attachment {};
  if (info.DepthStencilAttachment != nullptr && depth_view != VK_NULL_HANDLE) {
    depth_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    depth_attachment.imageView = depth_view;
    depth_attachment.imageLayout =
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depth_attachment.loadOp =
        ToVkLoadOp(info.DepthStencilAttachment->DepthLoadOp);
    depth_attachment.storeOp =
        info.DepthStencilAttachment->DepthStoreOp == StoreOp::Store
        ? VK_ATTACHMENT_STORE_OP_STORE
        : VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.clearValue.depthStencil.depth =
        info.DepthStencilAttachment->ClearDepthStencil.Depth;
    depth_attachment.clearValue.depthStencil.stencil =
        info.DepthStencilAttachment->ClearDepthStencil.Stencil;
  }

  VkRenderingInfo rendering_info {};
  rendering_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
  rendering_info.renderArea.offset = {.x = 0, .y = 0};
  rendering_info.renderArea.extent = {.width = info.Width,
                                      .height = info.Height};
  rendering_info.layerCount = 1;
  rendering_info.colorAttachmentCount = 1;
  rendering_info.pColorAttachments = &color_attachment;
  if (info.DepthStencilAttachment != nullptr && depth_view != VK_NULL_HANDLE) {
    rendering_info.pDepthAttachment = &depth_attachment;
  }

  vkCmdBeginRendering(m_CommandBuffer, &rendering_info);
}

void VulkanCommandBuffer::EndRenderPass()
{
  if (!m_InRenderPass) {
    return;
  }

  Logger::Trace("[Vulkan] End render pass");
  vkCmdEndRendering(m_CommandBuffer);

  // Resolve the color image for the post-pass transition
  VkImage color_image = VK_NULL_HANDLE;
  if (m_IsSwapchainTarget) {
    auto* swapchain = dynamic_cast<VulkanSwapchain*>(m_Device->GetSwapchain());
    color_image = swapchain->GetCurrentImage();
  } else if (m_CurrentRenderPass.RenderTarget != nullptr) {
    auto* rt =
        dynamic_cast<VulkanRenderTarget*>(m_CurrentRenderPass.RenderTarget);
    color_image = rt->GetColorImage();
  }

  // Transition: swapchain -> PRESENT_SRC, off-screen -> SHADER_READ_ONLY
  VkImageLayout new_layout = m_IsSwapchainTarget
      ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
      : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  VkAccessFlags dst_access =
      m_IsSwapchainTarget ? VkAccessFlags {0} : VK_ACCESS_SHADER_READ_BIT;
  VkPipelineStageFlags dst_stage = m_IsSwapchainTarget
      ? VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT
      : VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

  VkImageMemoryBarrier barrier {};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  barrier.newLayout = new_layout;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = color_image;
  barrier.subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                              .baseMipLevel = 0,
                              .levelCount = 1,
                              .baseArrayLayer = 0,
                              .layerCount = 1};
  barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  barrier.dstAccessMask = dst_access;

  vkCmdPipelineBarrier(m_CommandBuffer,
                       VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                       dst_stage,
                       0,
                       0,
                       nullptr,
                       0,
                       nullptr,
                       1,
                       &barrier);

  m_InRenderPass = false;
  m_IsSwapchainTarget = false;
  m_CurrentRenderPass = {};
}

auto VulkanCommandBuffer::GetHandle() const -> VkCommandBuffer
{
  return m_CommandBuffer;
}

void VulkanCommandBuffer::BindShaders(const RHIShaderModule* vertex_shader,
                                      const RHIShaderModule* fragment_shader)
{
  const auto* vk_vertex =
      dynamic_cast<const VulkanShaderModule*>(vertex_shader);
  const auto* vk_fragment =
      dynamic_cast<const VulkanShaderModule*>(fragment_shader);

  std::array<VkShaderStageFlagBits, 2> stages = {VK_SHADER_STAGE_VERTEX_BIT,
                                                 VK_SHADER_STAGE_FRAGMENT_BIT};
  std::array<VkShaderEXT, 2> shaders = {
      vk_vertex != nullptr ? vk_vertex->GetVkShaderEXT() : VK_NULL_HANDLE,
      vk_fragment != nullptr ? vk_fragment->GetVkShaderEXT() : VK_NULL_HANDLE};

  vkCmdBindShadersEXT(m_CommandBuffer, 2, stages.data(), shaders.data());

  const auto sample_mask = 0xFFFFFFFF;
  vkCmdSetSampleMaskEXT(m_CommandBuffer, VK_SAMPLE_COUNT_1_BIT, &sample_mask);
  vkCmdSetRasterizationSamplesEXT(m_CommandBuffer, VK_SAMPLE_COUNT_1_BIT);
  vkCmdSetRasterizerDiscardEnable(m_CommandBuffer, VK_FALSE);
  vkCmdSetAlphaToCoverageEnableEXT(m_CommandBuffer, VK_FALSE);
  VkPolygonMode vk_polygon_mode = VK_POLYGON_MODE_FILL;
  switch (m_PolygonMode) {
    case PolygonMode::Fill:
      vk_polygon_mode = VK_POLYGON_MODE_FILL;
      break;
    case PolygonMode::Line:
      vk_polygon_mode = VK_POLYGON_MODE_LINE;
      vkCmdSetLineWidth(m_CommandBuffer, 1.0F);
      break;
    case PolygonMode::Point:
      vk_polygon_mode = VK_POLYGON_MODE_POINT;
      break;
  }
  vkCmdSetPolygonModeEXT(m_CommandBuffer, vk_polygon_mode);
  vkCmdSetCullMode(m_CommandBuffer, VK_CULL_MODE_BACK_BIT);
  vkCmdSetFrontFace(m_CommandBuffer, VK_FRONT_FACE_COUNTER_CLOCKWISE);
  const VkBool32 depth_test_enable =
      m_CurrentRenderPass.DepthStencilAttachment != nullptr ? VK_TRUE
                                                            : VK_FALSE;
  vkCmdSetDepthTestEnable(m_CommandBuffer, depth_test_enable);
  vkCmdSetDepthWriteEnable(m_CommandBuffer, depth_test_enable);
  if (depth_test_enable == VK_TRUE) {
    vkCmdSetDepthCompareOp(m_CommandBuffer, VK_COMPARE_OP_LESS);
  }
  vkCmdSetDepthBiasEnable(m_CommandBuffer, VK_FALSE);
  vkCmdSetDepthBoundsTestEnable(m_CommandBuffer, VK_FALSE);
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

void VulkanCommandBuffer::BindVertexBuffer(const RHIBuffer& buffer,
                                           uint32_t binding)
{
  const auto& vk_buffer = dynamic_cast<const VulkanBuffer&>(buffer);
  VkBuffer vk_buf = vk_buffer.GetVkBuffer();
  const VkDeviceSize offset = 0;
  vkCmdBindVertexBuffers(m_CommandBuffer, binding, 1, &vk_buf, &offset);
}

void VulkanCommandBuffer::BindIndexBuffer(const RHIBuffer& buffer)
{
  const auto& vk_buffer = dynamic_cast<const VulkanBuffer&>(buffer);
  VkBuffer vk_buf = vk_buffer.GetVkBuffer();
  const VkDeviceSize offset = 0;
  vkCmdBindIndexBuffer(m_CommandBuffer, vk_buf, offset, VK_INDEX_TYPE_UINT32);
}

void VulkanCommandBuffer::SetVertexInput(const VertexInputLayout& layout)
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

void VulkanCommandBuffer::SetPrimitiveTopology(PrimitiveTopology topology)
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

void VulkanCommandBuffer::SetPolygonMode(PolygonMode mode)
{
  m_PolygonMode = mode;
}

void VulkanCommandBuffer::BindDescriptorSet(
    uint32_t set_index,
    const RHIDescriptorSet& descriptor_set,
    const RHIPipelineLayout& layout,
    std::span<const uint32_t> dynamic_offsets)
{
  const auto& vk_descriptor_set =
      dynamic_cast<const VulkanDescriptorSet&>(descriptor_set);
  const auto& vk_layout = dynamic_cast<const VulkanPipelineLayout&>(layout);

  VkDescriptorSet vk_set = vk_descriptor_set.GetVkDescriptorSet();

  vkCmdBindDescriptorSets(m_CommandBuffer,
                          VK_PIPELINE_BIND_POINT_GRAPHICS,
                          vk_layout.GetVkPipelineLayout(),
                          set_index,
                          1,
                          &vk_set,
                          static_cast<uint32_t>(dynamic_offsets.size()),
                          dynamic_offsets.data());
}

void VulkanCommandBuffer::Draw(uint32_t vertex_count,
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

void VulkanCommandBuffer::DrawIndexed(uint32_t index_count,
                                      uint32_t instance_count,
                                      uint32_t first_index,
                                      int32_t vertex_offset,
                                      uint32_t first_instance)
{
  vkCmdDrawIndexed(m_CommandBuffer,
                   index_count,
                   instance_count,
                   first_index,
                   vertex_offset,
                   first_instance);
}
