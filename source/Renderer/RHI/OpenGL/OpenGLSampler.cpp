#include <stdexcept>

#include "Renderer/RHI/OpenGL/OpenGLSampler.hpp"

#include "Core/Logger.hpp"

static auto ToGLFilter(Filter filter) -> GLint
{
  switch (filter) {
    case Filter::Nearest:
      return GL_NEAREST;
    case Filter::Linear:
      return GL_LINEAR;
  }
  return GL_LINEAR;
}

// Combines minification filter with mipmap filter
static auto ToGLMipFilter(Filter min_filter, Filter mip_filter) -> GLint
{
  if (min_filter == Filter::Nearest && mip_filter == Filter::Nearest) {
    return GL_NEAREST_MIPMAP_NEAREST;
  }
  if (min_filter == Filter::Linear && mip_filter == Filter::Nearest) {
    return GL_LINEAR_MIPMAP_NEAREST;
  }
  if (min_filter == Filter::Nearest && mip_filter == Filter::Linear) {
    return GL_NEAREST_MIPMAP_LINEAR;
  }
  return GL_LINEAR_MIPMAP_LINEAR;
}

static auto ToGLAddressMode(SamplerAddressMode mode) -> GLint
{
  switch (mode) {
    case SamplerAddressMode::Repeat:
      return GL_REPEAT;
    case SamplerAddressMode::MirroredRepeat:
      return GL_MIRRORED_REPEAT;
    case SamplerAddressMode::ClampToEdge:
      return GL_CLAMP_TO_EDGE;
    case SamplerAddressMode::ClampToBorder:
      return GL_CLAMP_TO_BORDER;
  }
  return GL_REPEAT;
}

OpenGLSampler::OpenGLSampler(const SamplerDesc& desc)
{
  glCreateSamplers(1, &m_Sampler);
  if (m_Sampler == 0) {
    throw std::runtime_error("Failed to create OpenGL sampler");
  }

  // Minification filter (includes mipmap behavior)
  glSamplerParameteri(m_Sampler,
                      GL_TEXTURE_MIN_FILTER,
                      ToGLMipFilter(desc.MinFilter, desc.MipFilter));

  // Magnification filter (no mipmaps involved)
  glSamplerParameteri(
      m_Sampler, GL_TEXTURE_MAG_FILTER, ToGLFilter(desc.MagFilter));

  // Texture wrapping modes
  glSamplerParameteri(
      m_Sampler, GL_TEXTURE_WRAP_S, ToGLAddressMode(desc.AddressModeU));
  glSamplerParameteri(
      m_Sampler, GL_TEXTURE_WRAP_T, ToGLAddressMode(desc.AddressModeV));

  // Anisotropic filtering (improves quality at oblique angles)
  if (desc.EnableAnisotropy && desc.MaxAnisotropy > 1.0F) {
    glSamplerParameterf(
        m_Sampler, GL_TEXTURE_MAX_ANISOTROPY, desc.MaxAnisotropy);
  }

  Logger::Trace("[OpenGL] Created sampler");
}

OpenGLSampler::~OpenGLSampler()
{
  if (m_Sampler != 0) {
    glDeleteSamplers(1, &m_Sampler);
  }
  Logger::Trace("[OpenGL] Destroyed sampler");
}
