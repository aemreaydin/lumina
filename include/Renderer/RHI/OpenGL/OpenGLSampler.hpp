#ifndef RENDERER_RHI_OPENGL_OPENGLSAMPLER_HPP
#define RENDERER_RHI_OPENGL_OPENGLSAMPLER_HPP

#include <glad/glad.h>

#include "Renderer/RHI/RHISampler.hpp"

class OpenGLSampler final : public RHISampler
{
public:
  explicit OpenGLSampler(const SamplerDesc& desc);

  OpenGLSampler(const OpenGLSampler&) = delete;
  OpenGLSampler(OpenGLSampler&&) = delete;
  auto operator=(const OpenGLSampler&) -> OpenGLSampler& = delete;
  auto operator=(OpenGLSampler&&) -> OpenGLSampler& = delete;
  ~OpenGLSampler() override;

  [[nodiscard]] auto GetGLSampler() const -> GLuint { return m_Sampler; }

private:
  GLuint m_Sampler {0};
};

#endif
