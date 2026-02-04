#include "Renderer/RHI/OpenGL/OpenGLRenderTarget.hpp"

#include <format>
#include <stdexcept>

#include "Core/Logger.hpp"
#include "Renderer/RHI/OpenGL/OpenGLTexture.hpp"

OpenGLRenderTarget::OpenGLRenderTarget(const RenderTargetDesc& desc)
    : m_Width(desc.Width)
    , m_Height(desc.Height)
{
  std::vector<GLenum> draw_buffers;
  for (size_t i = 0; i < desc.ColorFormats.size(); ++i) {
    TextureDesc color_desc;
    color_desc.Width = desc.Width;
    color_desc.Height = desc.Height;
    color_desc.Format = desc.ColorFormats[i];
    color_desc.Usage =
        TextureUsage::ColorAttachment | TextureUsage::Sampled;
    m_ColorTextures.push_back(std::make_unique<OpenGLTexture>(color_desc));
    draw_buffers.push_back(static_cast<GLenum>(GL_COLOR_ATTACHMENT0 + i));
  }

  if (desc.HasDepth) {
    TextureDesc depth_desc;
    depth_desc.Width = desc.Width;
    depth_desc.Height = desc.Height;
    depth_desc.Format = desc.DepthFormat;
    depth_desc.Usage = TextureUsage::DepthStencilAttachment;
    m_DepthTexture = std::make_unique<OpenGLTexture>(depth_desc);
  }

  glCreateFramebuffers(1, &m_Framebuffer);

  for (size_t i = 0; i < m_ColorTextures.size(); ++i) {
    glNamedFramebufferTexture(m_Framebuffer,
                              static_cast<GLenum>(GL_COLOR_ATTACHMENT0 + i),
                              m_ColorTextures[i]->GetGLTexture(),
                              0);
  }

  glNamedFramebufferDrawBuffers(m_Framebuffer,
                                static_cast<GLsizei>(draw_buffers.size()),
                                draw_buffers.data());

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

  Logger::Trace("[OpenGL] Created render target {}x{} with {} color attachment(s)",
                desc.Width,
                desc.Height,
                m_ColorTextures.size());
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

auto OpenGLRenderTarget::GetColorTexture(size_t index) -> RHITexture*
{
  if (index >= m_ColorTextures.size()) {
    return nullptr;
  }
  return m_ColorTextures[index].get();
}

auto OpenGLRenderTarget::GetColorTextureCount() const -> size_t
{
  return m_ColorTextures.size();
}

auto OpenGLRenderTarget::GetDepthTexture() -> RHITexture*
{
  return m_DepthTexture.get();
}
