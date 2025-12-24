#ifndef RENDERER_RHI_OPENGL_OPENGLCOMMANDBUFFER_HPP
#define RENDERER_RHI_OPENGL_OPENGLCOMMANDBUFFER_HPP

#include <glad/glad.h>

#include "Renderer/RHI/RHICommandBuffer.hpp"
#include "Renderer/RHI/RenderPassInfo.hpp"

class OpenGLCommandBuffer final : public RHICommandBuffer
{
public:
  OpenGLCommandBuffer() = default;
  OpenGLCommandBuffer(const OpenGLCommandBuffer&) = delete;
  OpenGLCommandBuffer(OpenGLCommandBuffer&&) = delete;
  auto operator=(const OpenGLCommandBuffer&) -> OpenGLCommandBuffer& = delete;
  auto operator=(OpenGLCommandBuffer&&) -> OpenGLCommandBuffer& = delete;
  ~OpenGLCommandBuffer() override;

  // OpenGL-specific lifecycle methods
  void Begin();
  void End();

  void BeginRenderPass(const RenderPassInfo& info);
  void EndRenderPass();

  static void ClearColor(float r, float g, float b, float a);

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

private:
  bool m_Recording {false};
  RenderPassInfo m_CurrentRenderPass {};
  GLuint m_CurrentProgram {0};
  GLuint m_VAO {0};
  GLenum m_PrimitiveMode {GL_TRIANGLES};
};

#endif
