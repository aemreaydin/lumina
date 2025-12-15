#ifndef RENDERER_RHI_OPENGL_OPENGLCOMMANDBUFFER_HPP
#define RENDERER_RHI_OPENGL_OPENGLCOMMANDBUFFER_HPP

#include <glad/glad.h>

#include "Renderer/RHI/OpenGL/OpenGLBackend.hpp"
#include "Renderer/RHI/RHICommandBuffer.hpp"
#include "Renderer/RHI/RHIVertexLayout.hpp"

class OpenGLShaderModule;
class OpenGLBuffer;

template<>
class RHICommandBuffer<OpenGLBackend>
{
public:
  RHICommandBuffer() = default;
  RHICommandBuffer(const RHICommandBuffer&) = delete;
  RHICommandBuffer(RHICommandBuffer&&) = delete;
  auto operator=(const RHICommandBuffer&) -> RHICommandBuffer& = delete;
  auto operator=(RHICommandBuffer&&) -> RHICommandBuffer& = delete;
  ~RHICommandBuffer();

  void Begin();
  void End();

  void BeginRenderPass(const RenderPassInfo& info);
  void EndRenderPass();

  static void ClearColor(float r, float g, float b, float a);

  // Drawing commands
  void BindShaders(const OpenGLShaderModule* vertex_shader,
                   const OpenGLShaderModule* fragment_shader);
  void BindVertexBuffer(const OpenGLBuffer& buffer, uint32_t binding = 0);
  void SetVertexInput(const VertexInputLayout& layout);
  void SetPrimitiveTopology(PrimitiveTopology topology);
  void Draw(uint32_t vertex_count,
            uint32_t instance_count = 1,
            uint32_t first_vertex = 0,
            uint32_t first_instance = 0) const;

  [[nodiscard]] static auto GetHandle() -> OpenGLBackend::CommandBufferHandle;

private:
  bool m_Recording {false};
  RenderPassInfo m_CurrentRenderPass {};
  GLuint m_CurrentProgram {0};
  GLuint m_VAO {0};
  GLenum m_PrimitiveMode {GL_TRIANGLES};
};

#endif
