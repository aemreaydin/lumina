#ifndef RENDERER_RHI_OPENGL_OPENGLRENDERTARGET_HPP
#define RENDERER_RHI_OPENGL_OPENGLRENDERTARGET_HPP

#include <memory>
#include <vector>

#include <glad/glad.h>

#include "Renderer/RHI/RHIRenderTarget.hpp"

class OpenGLTexture;

class OpenGLRenderTarget final : public RHIRenderTarget
{
public:
  explicit OpenGLRenderTarget(const RenderTargetDesc& desc);

  OpenGLRenderTarget(const OpenGLRenderTarget&) = delete;
  OpenGLRenderTarget(OpenGLRenderTarget&&) = delete;
  auto operator=(const OpenGLRenderTarget&) -> OpenGLRenderTarget& = delete;
  auto operator=(OpenGLRenderTarget&&) -> OpenGLRenderTarget& = delete;
  ~OpenGLRenderTarget() override;

  [[nodiscard]] auto GetWidth() const -> uint32_t override;
  [[nodiscard]] auto GetHeight() const -> uint32_t override;
  [[nodiscard]] auto GetColorTexture(size_t index = 0) -> RHITexture* override;
  [[nodiscard]] auto GetColorTextureCount() const -> size_t override;
  [[nodiscard]] auto GetDepthTexture() -> RHITexture* override;

  [[nodiscard]] auto GetFramebuffer() const -> GLuint { return m_Framebuffer; }

private:
  GLuint m_Framebuffer {0};
  uint32_t m_Width {0};
  uint32_t m_Height {0};
  std::vector<std::unique_ptr<OpenGLTexture>> m_ColorTextures;
  std::unique_ptr<OpenGLTexture> m_DepthTexture;
};

#endif
