#include <format>
#include <stdexcept>

#include "Renderer/RHI/OpenGL/OpenGLCommandBuffer.hpp"

#include <glad/glad.h>

#include "Core/Logger.hpp"
#include "Renderer/RHI/OpenGL/OpenGLBuffer.hpp"
#include "Renderer/RHI/OpenGL/OpenGLShaderModule.hpp"

void RHICommandBuffer<OpenGLBackend>::Begin()
{
  Logger::Trace("[OpenGL] Begin command buffer recording");
  m_Recording = true;
}

void RHICommandBuffer<OpenGLBackend>::End()
{
  Logger::Trace("[OpenGL] End command buffer recording");
  m_Recording = false;
}

void RHICommandBuffer<OpenGLBackend>::BeginRenderPass(
    const RenderPassInfo& info)
{
  Logger::Trace("[OpenGL] Begin render pass ({}x{})", info.Width, info.Height);
  m_CurrentRenderPass = info;

  // Set viewport
  glViewport(0,
             0,
             static_cast<GLsizei>(info.Width),
             static_cast<GLsizei>(info.Height));

  // Handle load operation
  if (info.ColorAttachment.ColorLoadOp == LoadOp::Clear) {
    const auto& clear = info.ColorAttachment.ClearColor;
    glClearColor(clear.R, clear.G, clear.B, clear.A);

    GLbitfield clear_flags = GL_COLOR_BUFFER_BIT;

    if (info.DepthStencilAttachment != nullptr) {
      if (info.DepthStencilAttachment->DepthLoadOp == LoadOp::Clear) {
        glClearDepth(info.DepthStencilAttachment->ClearDepthStencil.Depth);
        clear_flags |= GL_DEPTH_BUFFER_BIT;
      }
      if (info.DepthStencilAttachment->StencilLoadOp == LoadOp::Clear) {
        glClearStencil(static_cast<GLint>(
            info.DepthStencilAttachment->ClearDepthStencil.Stencil));
        clear_flags |= GL_STENCIL_BUFFER_BIT;
      }
    }

    glClear(clear_flags);
  }
}

void RHICommandBuffer<OpenGLBackend>::EndRenderPass()
{
  Logger::Trace("[OpenGL] End render pass");
  m_CurrentRenderPass = {};
}

void RHICommandBuffer<OpenGLBackend>::ClearColor(float r,
                                                 float g,
                                                 float b,
                                                 float a)
{
  glClearColor(r, g, b, a);
  glClear(GL_COLOR_BUFFER_BIT);
}

auto RHICommandBuffer<OpenGLBackend>::GetHandle()
    -> OpenGLBackend::CommandBufferHandle
{
  return nullptr;
}

RHICommandBuffer<OpenGLBackend>::~RHICommandBuffer()
{
  if (m_CurrentProgram != 0) {
    glDeleteProgram(m_CurrentProgram);
  }
  if (m_VAO != 0) {
    glDeleteVertexArrays(1, &m_VAO);
  }
}

void RHICommandBuffer<OpenGLBackend>::BindShaders(
    const OpenGLShaderModule* vertex_shader,
    const OpenGLShaderModule* fragment_shader)
{
  if (vertex_shader == nullptr || fragment_shader == nullptr) {
    throw std::runtime_error("Both vertex and fragment shaders are required");
  }

  // Delete old program if exists
  if (m_CurrentProgram != 0) {
    glDeleteProgram(m_CurrentProgram);
  }

  // Create and link program
  m_CurrentProgram = glCreateProgram();
  glAttachShader(m_CurrentProgram, vertex_shader->GetGLShader());
  glAttachShader(m_CurrentProgram, fragment_shader->GetGLShader());
  glLinkProgram(m_CurrentProgram);

  GLint success = 0;
  glGetProgramiv(m_CurrentProgram, GL_LINK_STATUS, &success);
  if (success == 0) {
    GLint log_length = 0;
    glGetProgramiv(m_CurrentProgram, GL_INFO_LOG_LENGTH, &log_length);

    std::string info_log(static_cast<size_t>(log_length), '\0');
    glGetProgramInfoLog(m_CurrentProgram, log_length, nullptr, info_log.data());

    glDeleteProgram(m_CurrentProgram);
    m_CurrentProgram = 0;
    throw std::runtime_error(
        std::format("Failed to link shader program: {}", info_log));
  }

  glUseProgram(m_CurrentProgram);
  Logger::Trace("[OpenGL] Bound shader program");
}

void RHICommandBuffer<OpenGLBackend>::BindVertexBuffer(
    const OpenGLBuffer& buffer, uint32_t /*binding*/)
{
  if (m_VAO == 0) {
    glGenVertexArrays(1, &m_VAO);
  }
  glBindVertexArray(m_VAO);
  glBindBuffer(GL_ARRAY_BUFFER, buffer.GetGLBuffer());
}

void RHICommandBuffer<OpenGLBackend>::BindIndexBuffer(
    const OpenGLBuffer& buffer)
{
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer.GetGLBuffer());
}

void RHICommandBuffer<OpenGLBackend>::SetVertexInput(
    const VertexInputLayout& layout)
{
  if (m_VAO == 0) {
    glGenVertexArrays(1, &m_VAO);
    glBindVertexArray(m_VAO);
  }

  for (const auto& attr : layout.Attributes) {
    GLint size = 3;
    GLenum type = GL_FLOAT;

    switch (attr.Format) {
      case VertexFormat::Float:
        size = 1;
        type = GL_FLOAT;
        break;
      case VertexFormat::Float2:
        size = 2;
        type = GL_FLOAT;
        break;
      case VertexFormat::Float3:
        size = 3;
        type = GL_FLOAT;
        break;
      case VertexFormat::Float4:
        size = 4;
        type = GL_FLOAT;
        break;
      default:
        size = 3;
        type = GL_FLOAT;
        break;
    }

    glEnableVertexAttribArray(attr.Location);
    glVertexAttribPointer(
        attr.Location,
        size,
        type,
        GL_FALSE,
        static_cast<GLsizei>(layout.Stride),
        reinterpret_cast<const void*>(static_cast<uintptr_t>(attr.Offset)));
  }
}

void RHICommandBuffer<OpenGLBackend>::SetPrimitiveTopology(
    PrimitiveTopology topology)
{
  switch (topology) {
    case PrimitiveTopology::TriangleList:
      m_PrimitiveMode = GL_TRIANGLES;
      break;
    case PrimitiveTopology::TriangleStrip:
      m_PrimitiveMode = GL_TRIANGLE_STRIP;
      break;
    case PrimitiveTopology::LineList:
      m_PrimitiveMode = GL_LINES;
      break;
    case PrimitiveTopology::LineStrip:
      m_PrimitiveMode = GL_LINE_STRIP;
      break;
    case PrimitiveTopology::PointList:
      m_PrimitiveMode = GL_POINTS;
      break;
  }
}

void RHICommandBuffer<OpenGLBackend>::Draw(uint32_t vertex_count,
                                           uint32_t instance_count,
                                           uint32_t first_vertex,
                                           uint32_t first_instance) const
{
  if (instance_count == 1 && first_instance == 0) {
    glDrawArrays(m_PrimitiveMode,
                 static_cast<GLint>(first_vertex),
                 static_cast<GLsizei>(vertex_count));
  } else {
    glDrawArraysInstancedBaseInstance(m_PrimitiveMode,
                                      static_cast<GLint>(first_vertex),
                                      static_cast<GLsizei>(vertex_count),
                                      static_cast<GLsizei>(instance_count),
                                      first_instance);
  }
}

void RHICommandBuffer<OpenGLBackend>::DrawIndexed(uint32_t index_count,
                                                  uint32_t instance_count,
                                                  uint32_t first_instance,
                                                  const void* indices) const
{
  if (instance_count == 1 && first_instance == 0) {
    glDrawElements(m_PrimitiveMode,
                   static_cast<GLint>(index_count),
                   GL_UNSIGNED_INT,
                   indices);
  } else {
    glDrawElementsInstanced(m_PrimitiveMode,
                            static_cast<GLint>(index_count),
                            GL_UNSIGNED_INT,
                            indices,
                            static_cast<GLsizei>(instance_count));
  }
}
