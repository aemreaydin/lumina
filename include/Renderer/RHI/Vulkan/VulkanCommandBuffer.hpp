#ifndef RENDERER_RHI_VULKAN_VULKANCOMMANDBUFFER_HPP
#define RENDERER_RHI_VULKAN_VULKANCOMMANDBUFFER_HPP

#include <volk.h>

#include "Renderer/RHI/RHICommandBuffer.hpp"
#include "Renderer/RHI/RHIVertexLayout.hpp"
#include "Renderer/RHI/Vulkan/VulkanBackend.hpp"

class VulkanDevice;
class VulkanSwapchain;
class VulkanShaderModule;
class VulkanBuffer;

template<>
class RHICommandBuffer<VulkanBackend>
{
public:
  RHICommandBuffer() = default;
  RHICommandBuffer(const RHICommandBuffer&) = delete;
  RHICommandBuffer(RHICommandBuffer&&) = delete;
  auto operator=(const RHICommandBuffer&) -> RHICommandBuffer& = delete;
  auto operator=(RHICommandBuffer&&) -> RHICommandBuffer& = delete;
  ~RHICommandBuffer() = default;

  void Allocate(const VulkanDevice& device, VkCommandPool pool);
  void Free(const VulkanDevice& device, VkCommandPool pool);
  void Begin();
  void End();

  void BeginRenderPass(const VulkanSwapchain& swapchain,
                       const RenderPassInfo& info);
  void EndRenderPass(const VulkanSwapchain& swapchain);

  void ClearColor(
      const VulkanSwapchain& swapchain, float r, float g, float b, float a);

  // Drawing commands
  void BindShaders(const VulkanShaderModule* vertex_shader,
                   const VulkanShaderModule* fragment_shader);
  void BindVertexBuffer(const VulkanBuffer& buffer, uint32_t binding = 0);
  void BindIndexBuffer(const VulkanBuffer& buffer);

  void SetVertexInput(const VertexInputLayout& layout);
  void SetPrimitiveTopology(PrimitiveTopology topology);
  void Draw(uint32_t vertex_count,
            uint32_t instance_count = 1,
            uint32_t first_vertex = 0,
            uint32_t first_instance = 0);
  void DrawIndexed(uint32_t index_count,
                   uint32_t instance_count = 0,
                   uint32_t first_index = 0,
                   uint32_t vertex_offset = 0,
                   uint32_t first_instance = 0);

  [[nodiscard]] auto GetHandle() -> VkCommandBuffer;

private:
  VkCommandBuffer m_CommandBuffer {VK_NULL_HANDLE};
  bool m_Recording {false};
  bool m_InRenderPass {false};
  RenderPassInfo m_CurrentRenderPass {};
};

#endif
