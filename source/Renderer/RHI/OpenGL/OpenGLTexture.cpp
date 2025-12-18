#include <stdexcept>

#include "Renderer/RHI/OpenGL/OpenGLTexture.hpp"

#include "Core/Logger.hpp"

struct GLFormatInfo
{
  GLenum InternalFormat;  // How GPU stores the texture
  GLenum Format;  // Component order of input data
  GLenum Type;  // Data type of each component
};

static auto GetGLFormatInfo(TextureFormat format) -> GLFormatInfo
{
  switch (format) {
    case TextureFormat::R8Unorm:
      return {
          .InternalFormat = GL_R8, .Format = GL_RED, .Type = GL_UNSIGNED_BYTE};
    case TextureFormat::RG8Unorm:
      return {
          .InternalFormat = GL_RG8, .Format = GL_RG, .Type = GL_UNSIGNED_BYTE};
    case TextureFormat::RGB8Unorm:
      return {.InternalFormat = GL_RGB8,
              .Format = GL_RGB,
              .Type = GL_UNSIGNED_BYTE};
    case TextureFormat::RGB8Srgb:
      return {.InternalFormat = GL_SRGB8,
              .Format = GL_RGB,
              .Type = GL_UNSIGNED_BYTE};
    case TextureFormat::RGBA8Unorm:
      return {.InternalFormat = GL_RGBA8,
              .Format = GL_RGBA,
              .Type = GL_UNSIGNED_BYTE};
    case TextureFormat::RGBA8Srgb:
      return {.InternalFormat = GL_SRGB8_ALPHA8,
              .Format = GL_RGBA,
              .Type = GL_UNSIGNED_BYTE};
    case TextureFormat::BGRA8Unorm:
      return {.InternalFormat = GL_RGBA8,
              .Format = GL_BGRA,
              .Type = GL_UNSIGNED_BYTE};
    case TextureFormat::RGBA16F:
      return {.InternalFormat = GL_RGBA16F,
              .Format = GL_RGBA,
              .Type = GL_HALF_FLOAT};
    case TextureFormat::RGBA32F:
      return {
          .InternalFormat = GL_RGBA32F, .Format = GL_RGBA, .Type = GL_FLOAT};
    case TextureFormat::Depth24Stencil8:
      return {.InternalFormat = GL_DEPTH24_STENCIL8,
              .Format = GL_DEPTH_STENCIL,
              .Type = GL_UNSIGNED_INT_24_8};
    case TextureFormat::Depth32F:
      return {.InternalFormat = GL_DEPTH_COMPONENT32F,
              .Format = GL_DEPTH_COMPONENT,
              .Type = GL_FLOAT};
  }
  return {
      .InternalFormat = GL_RGBA8, .Format = GL_RGBA, .Type = GL_UNSIGNED_BYTE};
}

OpenGLTexture::OpenGLTexture(const TextureDesc& desc)
    : m_Width(desc.Width)
    , m_Height(desc.Height)
    , m_Format(desc.Format)
{
  auto format_info = GetGLFormatInfo(desc.Format);
  m_GLInternalFormat = format_info.InternalFormat;
  m_GLFormat = format_info.Format;
  m_GLType = format_info.Type;

  glCreateTextures(GL_TEXTURE_2D, 1, &m_Texture);
  if (m_Texture == 0) {
    throw std::runtime_error("Failed to create OpenGL texture");
  }

  // Allocate immutable storage for the texture
  glTextureStorage2D(m_Texture,
                     static_cast<GLsizei>(desc.MipLevels),
                     m_GLInternalFormat,
                     static_cast<GLsizei>(desc.Width),
                     static_cast<GLsizei>(desc.Height));

  Logger::Trace("[OpenGL] Created texture {}x{}", desc.Width, desc.Height);
}

OpenGLTexture::~OpenGLTexture()
{
  if (m_Texture != 0) {
    glDeleteTextures(1, &m_Texture);
  }
  Logger::Trace("[OpenGL] Destroyed texture");
}

auto OpenGLTexture::GetWidth() const -> uint32_t
{
  return m_Width;
}

auto OpenGLTexture::GetHeight() const -> uint32_t
{
  return m_Height;
}

auto OpenGLTexture::GetFormat() const -> TextureFormat
{
  return m_Format;
}

void OpenGLTexture::Upload(const void* data, size_t /*size*/)
{
  // Upload pixel data to the texture
  glTextureSubImage2D(m_Texture,
                      0,  // mip level
                      0,  // x offset
                      0,  // y offset
                      static_cast<GLsizei>(m_Width),
                      static_cast<GLsizei>(m_Height),
                      m_GLFormat,
                      m_GLType,
                      data);

  Logger::Trace("[OpenGL] Uploaded texture data");
}
