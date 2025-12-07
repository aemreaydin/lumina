#ifndef RENDERER_RHI_COMMANDBUFFER_HPP
#define RENDERER_RHI_COMMANDBUFFER_HPP

#include "RenderPassInfo.hpp"

// Template-based command buffer for zero-overhead command recording
// Specialized for each backend (OpenGL, Vulkan)
template<typename Backend>
class RHICommandBuffer
{
public:
  RHICommandBuffer() = default;
  RHICommandBuffer(const RHICommandBuffer&) = delete;
  RHICommandBuffer(RHICommandBuffer&&) = delete;
  auto operator=(const RHICommandBuffer&) -> RHICommandBuffer& = delete;
  auto operator=(RHICommandBuffer&&) -> RHICommandBuffer& = delete;
  ~RHICommandBuffer() = default;

  // Command recording
  void Begin();
  void End();

  // Render pass
  void BeginRenderPass(const RenderPassInfo& info);
  void EndRenderPass();

  // Clear commands
  void ClearColor(float r, float g, float b, float a);

  // Get native handle
  [[nodiscard]] auto GetHandle() -> typename Backend::CommandBufferHandle;

private:
  typename Backend::CommandBufferHandle m_Handle {};
  bool m_Recording {false};
  RenderPassInfo m_CurrentRenderPass {};
};

#endif
