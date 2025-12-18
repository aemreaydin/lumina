#ifndef RENDERER_RHI_OPENGL_OPENGLTEXTURE_HPP
#define RENDERER_RHI_OPENGL_OPENGLTEXTURE_HPP

#include <glad/glad.h>

#include "Renderer/RHI/RHITexture.hpp"

class OpenGLTexture final : public RHITexture
{
public:
  explicit OpenGLTexture(const TextureDesc& desc);

  OpenGLTexture(const OpenGLTexture&) = delete;
  OpenGLTexture(OpenGLTexture&&) = delete;
  auto operator=(const OpenGLTexture&) -> OpenGLTexture& = delete;
  auto operator=(OpenGLTexture&&) -> OpenGLTexture& = delete;
  ~OpenGLTexture() override;

  [[nodiscard]] auto GetWidth() const -> uint32_t override;
  [[nodiscard]] auto GetHeight() const -> uint32_t override;
  [[nodiscard]] auto GetFormat() const -> TextureFormat override;
  void Upload(const void* data, size_t size) override;

  [[nodiscard]] auto GetGLTexture() const -> GLuint { return m_Texture; }

private:
  GLuint m_Texture {0};
  uint32_t m_Width {0};
  uint32_t m_Height {0};
  TextureFormat m_Format {TextureFormat::RGBA8Unorm};
  GLenum m_GLInternalFormat {GL_RGBA8};  // GPU storage format
  GLenum m_GLFormat {GL_RGBA};  // Input data component order
  GLenum m_GLType {GL_UNSIGNED_BYTE};  // Input data type per component
};

#endif
