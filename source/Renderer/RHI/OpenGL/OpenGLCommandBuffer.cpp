#include <format>
#include <stdexcept>

#include "Renderer/RHI/OpenGL/OpenGLCommandBuffer.hpp"

#include <glad/glad.h>

#include "Core/Logger.hpp"
#include "Renderer/RHI/OpenGL/OpenGLBuffer.hpp"
#include "Renderer/RHI/OpenGL/OpenGLDescriptorSet.hpp"
#include "Renderer/RHI/OpenGL/OpenGLShaderModule.hpp"

void OpenGLCommandBuffer::Begin()
{
  Logger::Trace("[OpenGL] Begin command buffer recording");
  m_Recording = true;
}

void OpenGLCommandBuffer::End()
{
  Logger::Trace("[OpenGL] End command buffer recording");
  m_Recording = false;
}

void OpenGLCommandBuffer::BeginRenderPass(const RenderPassInfo& info)
{
  Logger::Trace("[OpenGL] Begin render pass ({}x{})", info.Width, info.Height);
  m_CurrentRenderPass = info;

  glViewport(0,
             0,
             static_cast<GLsizei>(info.Width),
             static_cast<GLsizei>(info.Height));
  glFrontFace(GL_CCW);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);

  if (info.DepthStencilAttachment != nullptr) {
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
  } else {
    glDisable(GL_DEPTH_TEST);
  }

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

void OpenGLCommandBuffer::EndRenderPass()
{
  Logger::Trace("[OpenGL] End render pass");
  m_CurrentRenderPass = {};
}

void OpenGLCommandBuffer::ClearColor(float r, float g, float b, float a)
{
  glClearColor(r, g, b, a);
  glClear(GL_COLOR_BUFFER_BIT);
}

OpenGLCommandBuffer::~OpenGLCommandBuffer()
{
  if (m_CurrentProgram != 0) {
    glDeleteProgram(m_CurrentProgram);
  }
  if (m_VAO != 0) {
    glDeleteVertexArrays(1, &m_VAO);
  }
}

void OpenGLCommandBuffer::BindShaders(const RHIShaderModule* vertex_shader,
                                      const RHIShaderModule* fragment_shader)
{
  const auto* gl_vertex =
      dynamic_cast<const OpenGLShaderModule*>(vertex_shader);
  const auto* gl_fragment =
      dynamic_cast<const OpenGLShaderModule*>(fragment_shader);

  if (gl_vertex == nullptr || gl_fragment == nullptr) {
    throw std::runtime_error("Both vertex and fragment shaders are required");
  }

  if (m_CurrentProgram != 0) {
    glDeleteProgram(m_CurrentProgram);
  }

  m_CurrentProgram = glCreateProgram();
  glAttachShader(m_CurrentProgram, gl_vertex->GetGLShader());
  glAttachShader(m_CurrentProgram, gl_fragment->GetGLShader());
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

void OpenGLCommandBuffer::BindVertexBuffer(const RHIBuffer& buffer,
                                           uint32_t /*binding*/)
{
  const auto& gl_buffer = dynamic_cast<const OpenGLBuffer&>(buffer);

  if (m_VAO == 0) {
    glGenVertexArrays(1, &m_VAO);
  }
  glBindVertexArray(m_VAO);
  glBindBuffer(GL_ARRAY_BUFFER, gl_buffer.GetGLBuffer());

  if (m_HasPendingLayout) {
    applyVertexLayout();
  }
}

void OpenGLCommandBuffer::BindIndexBuffer(const RHIBuffer& buffer)
{
  const auto& gl_buffer = dynamic_cast<const OpenGLBuffer&>(buffer);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gl_buffer.GetGLBuffer());
}

void OpenGLCommandBuffer::SetVertexInput(const VertexInputLayout& layout)
{
  m_PendingLayout = layout;
  m_HasPendingLayout = true;
}

void OpenGLCommandBuffer::applyVertexLayout()
{
  for (const auto& attr : m_PendingLayout.Attributes) {
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
        static_cast<GLsizei>(m_PendingLayout.Stride),
        reinterpret_cast<const void*>(static_cast<uintptr_t>(attr.Offset)));
  }
}

void OpenGLCommandBuffer::SetPrimitiveTopology(PrimitiveTopology topology)
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

void OpenGLCommandBuffer::SetPolygonMode(PolygonMode mode)
{
  switch (mode) {
    case PolygonMode::Fill:
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
      break;
    case PolygonMode::Line:
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
      break;
    case PolygonMode::Point:
      glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
      break;
  }
}

void OpenGLCommandBuffer::BindDescriptorSet(
    uint32_t set_index,
    const RHIDescriptorSet& descriptor_set,
    [[maybe_unused]] const RHIPipelineLayout& layout,
    std::span<const uint32_t> dynamic_offsets)
{
  const auto& gl_descriptor_set =
      dynamic_cast<const OpenGLDescriptorSet&>(descriptor_set);
  gl_descriptor_set.Bind(set_index, dynamic_offsets);
}

void OpenGLCommandBuffer::Draw(uint32_t vertex_count,
                               uint32_t instance_count,
                               uint32_t first_vertex,
                               uint32_t first_instance)
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

void OpenGLCommandBuffer::DrawIndexed(uint32_t index_count,
                                      uint32_t instance_count,
                                      uint32_t first_index,
                                      int32_t /*vertex_offset*/,
                                      uint32_t first_instance)
{
  const auto* indices = reinterpret_cast<const void*>(
      static_cast<uintptr_t>(first_index) * sizeof(uint32_t));

  if (instance_count <= 1 && first_instance == 0) {
    glDrawElements(m_PrimitiveMode,
                   static_cast<GLsizei>(index_count),
                   GL_UNSIGNED_INT,
                   indices);
  } else {
    glDrawElementsInstancedBaseInstance(m_PrimitiveMode,
                                        static_cast<GLsizei>(index_count),
                                        GL_UNSIGNED_INT,
                                        indices,
                                        static_cast<GLsizei>(instance_count),
                                        first_instance);
  }
}
