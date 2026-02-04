#include "Renderer/RHI/OpenGL/OpenGLRenderTarget.hpp"

#include <format>
#include <stdexcept>

#include "Core/Logger.hpp"
#include "Renderer/RHI/OpenGL/OpenGLTexture.hpp"

OpenGLRenderTarget::OpenGLRenderTarget(const RenderTargetDesc& desc)
    : m_Width(desc.Width)
    , m_Height(desc.Height)
{
  TextureDesc color_desc;
  color_desc.Width = desc.Width;
  color_desc.Height = desc.Height;
  color_desc.Format = desc.ColorFormat;
  color_desc.Usage =
      TextureUsage::ColorAttachment | TextureUsage::Sampled;
  m_ColorTexture = std::make_unique<OpenGLTexture>(color_desc);

  if (desc.HasDepth) {
    TextureDesc depth_desc;
    depth_desc.Width = desc.Width;
    depth_desc.Height = desc.Height;
    depth_desc.Format = desc.DepthFormat;
    depth_desc.Usage = TextureUsage::DepthStencilAttachment;
    m_DepthTexture = std::make_unique<OpenGLTexture>(depth_desc);
  }

  glCreateFramebuffers(1, &m_Framebuffer);

  glNamedFramebufferTexture(
      m_Framebuffer, GL_COLOR_ATTACHMENT0, m_ColorTexture->GetGLTexture(), 0);

  if (m_DepthTexture) {
    GLenum attachment = (desc.DepthFormat == TextureFormat::Depth24Stencil8)
        ? GL_DEPTH_STENCIL_ATTACHMENT
        : GL_DEPTH_ATTACHMENT;
    glNamedFramebufferTexture(
        m_Framebuffer, attachment, m_DepthTexture->GetGLTexture(), 0);
  }

  GLenum status = glCheckNamedFramebufferStatus(m_Framebuffer, GL_FRAMEBUFFER);
  if (status != GL_FRAMEBUFFER_COMPLETE) {
    throw std::runtime_error(
        std::format("OpenGL framebuffer incomplete: {:#x}", status));
  }

  Logger::Trace(
      "[OpenGL] Created render target {}x{}", desc.Width, desc.Height);
}

OpenGLRenderTarget::~OpenGLRenderTarget()
{
  if (m_Framebuffer != 0) {
    glDeleteFramebuffers(1, &m_Framebuffer);
  }
  Logger::Trace("[OpenGL] Destroyed render target");
}

auto OpenGLRenderTarget::GetWidth() const -> uint32_t
{
  return m_Width;
}

auto OpenGLRenderTarget::GetHeight() const -> uint32_t
{
  return m_Height;
}

auto OpenGLRenderTarget::GetColorTexture() -> RHITexture*
{
  return m_ColorTexture.get();
}

auto OpenGLRenderTarget::GetDepthTexture() -> RHITexture*
{
  return m_DepthTexture.get();
}
