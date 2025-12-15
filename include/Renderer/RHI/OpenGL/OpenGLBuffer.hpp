#ifndef RENDERER_RHI_OPENGL_OPENGLBUFFER_HPP
#define RENDERER_RHI_OPENGL_OPENGLBUFFER_HPP

#include <glad/glad.h>

#include "Renderer/RHI/RHIBuffer.hpp"

class OpenGLBuffer final : public RHIBuffer
{
public:
  explicit OpenGLBuffer(const BufferDesc& desc);

  OpenGLBuffer(const OpenGLBuffer&) = delete;
  OpenGLBuffer(OpenGLBuffer&&) = delete;
  auto operator=(const OpenGLBuffer&) -> OpenGLBuffer& = delete;
  auto operator=(OpenGLBuffer&&) -> OpenGLBuffer& = delete;
  ~OpenGLBuffer() override;

  [[nodiscard]] auto Map() -> void* override;
  void Unmap() override;
  void Upload(const void* data, size_t size, size_t offset) override;
  [[nodiscard]] auto GetSize() const -> size_t override;

  [[nodiscard]] auto GetGLBuffer() const -> GLuint { return m_Buffer; }

  [[nodiscard]] auto GetTarget() const -> GLenum { return m_Target; }

private:
  GLuint m_Buffer {0};
  GLenum m_Target {GL_ARRAY_BUFFER};
  size_t m_Size {0};
  bool m_Mapped {false};
};

#endif
