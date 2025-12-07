#include "Renderer/RHI/OpenGL/OpenGLCommandBuffer.hpp"

#include <glad/glad.h>

#include "Core/Logger.hpp"

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
