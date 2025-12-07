#ifndef RENDERER_RHI_VULKAN_VULKANCOMMANDBUFFER_HPP
#define RENDERER_RHI_VULKAN_VULKANCOMMANDBUFFER_HPP

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_handles.hpp>

#include "Renderer/RHI/RHICommandBuffer.hpp"
#include "Renderer/RHI/Vulkan/VulkanBackend.hpp"

class VulkanDevice;
class VulkanSwapchain;

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

  void Allocate(const VulkanDevice& device, const vk::CommandPool& pool);
  void Free(const VulkanDevice& device, const vk::CommandPool& pool);
  void Begin();
  void End();

  void BeginRenderPass(const VulkanSwapchain& swapchain,
                       const RenderPassInfo& info);
  void EndRenderPass(const VulkanSwapchain& swapchain);

  void ClearColor(
      const VulkanSwapchain& swapchain, float r, float g, float b, float a);

  [[nodiscard]] auto GetHandle() -> vk::CommandBuffer;

private:
  vk::CommandBuffer m_CommandBuffer;
  bool m_Recording {false};
  bool m_InRenderPass {false};
  RenderPassInfo m_CurrentRenderPass {};
};

#endif
