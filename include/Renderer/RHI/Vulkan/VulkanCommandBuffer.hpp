#ifndef RENDERER_RHI_VULKAN_VULKANCOMMANDBUFFER_HPP
#define RENDERER_RHI_VULKAN_VULKANCOMMANDBUFFER_HPP

#include <volk.h>

#include "Renderer/RHI/RHICommandBuffer.hpp"
#include "Renderer/RHI/RenderPassInfo.hpp"

class VulkanDevice;
class VulkanSwapchain;

class VulkanCommandBuffer final : public RHICommandBuffer
{
public:
  VulkanCommandBuffer() = default;
  VulkanCommandBuffer(const VulkanCommandBuffer&) = delete;
  VulkanCommandBuffer(VulkanCommandBuffer&&) = delete;
  auto operator=(const VulkanCommandBuffer&) -> VulkanCommandBuffer& = delete;
  auto operator=(VulkanCommandBuffer&&) -> VulkanCommandBuffer& = delete;
  ~VulkanCommandBuffer() override = default;

  // Vulkan-specific lifecycle methods
  void Allocate(const VulkanDevice& device, VkCommandPool pool);
  void Free(const VulkanDevice& device, VkCommandPool pool);
  void Begin();
  void End();

  // Render pass (Vulkan-specific, needs swapchain)
  void BeginRenderPass(const VulkanSwapchain& swapchain,
                       const RenderPassInfo& info);
  void EndRenderPass(const VulkanSwapchain& swapchain);

  void ClearColor(const VulkanSwapchain& swapchain,
                  float r,
                  float g,
                  float b,
                  float a);

  // RHICommandBuffer interface (drawing commands)
  void BindShaders(const RHIShaderModule* vertex_shader,
                   const RHIShaderModule* fragment_shader) override;
  void BindVertexBuffer(const RHIBuffer& buffer, uint32_t binding) override;
  void BindIndexBuffer(const RHIBuffer& buffer) override;
  void SetVertexInput(const VertexInputLayout& layout) override;
  void SetPrimitiveTopology(PrimitiveTopology topology) override;
  void BindDescriptorSet(uint32_t set_index,
                         const RHIDescriptorSet& descriptor_set,
                         const RHIPipelineLayout& layout) override;
  void Draw(uint32_t vertex_count,
            uint32_t instance_count,
            uint32_t first_vertex,
            uint32_t first_instance) override;
  void DrawIndexed(uint32_t index_count,
                   uint32_t instance_count,
                   uint32_t first_index,
                   int32_t vertex_offset,
                   uint32_t first_instance) override;
  void PushConstants(const RHIPipelineLayout& layout,
                     const PushConstant& push_constant,
                     const void* data) override;

  [[nodiscard]] auto GetHandle() const -> VkCommandBuffer;

private:
  VkCommandBuffer m_CommandBuffer {VK_NULL_HANDLE};
  bool m_Recording {false};
  bool m_InRenderPass {false};
  RenderPassInfo m_CurrentRenderPass {};
};

#endif
