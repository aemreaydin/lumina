#ifndef RENDERER_RHI_OPENGL_OPENGLCOMMANDBUFFER_HPP
#define RENDERER_RHI_OPENGL_OPENGLCOMMANDBUFFER_HPP

#include "Renderer/RHI/OpenGL/OpenGLBackend.hpp"
#include "Renderer/RHI/RHICommandBuffer.hpp"

template<>
class RHICommandBuffer<OpenGLBackend>
{
public:
  RHICommandBuffer() = default;
  RHICommandBuffer(const RHICommandBuffer&) = delete;
  RHICommandBuffer(RHICommandBuffer&&) = delete;
  auto operator=(const RHICommandBuffer&) -> RHICommandBuffer& = delete;
  auto operator=(RHICommandBuffer&&) -> RHICommandBuffer& = delete;
  ~RHICommandBuffer() = default;

  void Begin();
  void End();

  void BeginRenderPass(const RenderPassInfo& info);
  void EndRenderPass();

  static void ClearColor(float r, float g, float b, float a);

  [[nodiscard]] static auto GetHandle() -> OpenGLBackend::CommandBufferHandle;

private:
  bool m_Recording {false};
  RenderPassInfo m_CurrentRenderPass {};
};

#endif
