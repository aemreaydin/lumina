#include <stdexcept>

#include "Renderer/RHI/OpenGL/OpenGLBuffer.hpp"

#include "Core/Logger.hpp"
#include "Renderer/RHI/RHIBuffer.hpp"

OpenGLBuffer::OpenGLBuffer(const BufferDesc& desc)
    : m_Size(desc.Size)
{
  // Determine primary target based on usage
  if ((desc.Usage & BufferUsage::Index) != BufferUsage {}) {
    m_Target = GL_ELEMENT_ARRAY_BUFFER;
  } else if ((desc.Usage & BufferUsage::Uniform) != BufferUsage {}) {
    m_Target = GL_UNIFORM_BUFFER;
  } else {
    m_Target = GL_ARRAY_BUFFER;
  }

  glGenBuffers(1, &m_Buffer);
  if (m_Buffer == 0) {
    throw std::runtime_error("Failed to create OpenGL buffer");
  }

  glBindBuffer(m_Target, m_Buffer);

  const GLenum usage_hint = desc.CPUVisible ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW;

  glBufferData(
      m_Target, static_cast<GLsizeiptr>(desc.Size), nullptr, usage_hint);

  glBindBuffer(m_Target, 0);

  Logger::Trace("[OpenGL] Created buffer with size {}", desc.Size);
}

OpenGLBuffer::~OpenGLBuffer()
{
  if (m_Mapped) {
    Unmap();
  }

  if (m_Buffer != 0) {
    glDeleteBuffers(1, &m_Buffer);
  }

  Logger::Trace("[OpenGL] Destroyed buffer");
}

auto OpenGLBuffer::Map() -> void*
{
  if (m_Mapped) {
    glBindBuffer(m_Target, m_Buffer);
    return glMapBuffer(m_Target, GL_READ_WRITE);
  }

  glBindBuffer(m_Target, m_Buffer);
  void* ptr = glMapBuffer(m_Target, GL_READ_WRITE);  // NOLINT

  if (ptr == nullptr) {
    glBindBuffer(m_Target, 0);
    throw std::runtime_error("Failed to map OpenGL buffer");
  }

  m_Mapped = true;
  return ptr;
}

void OpenGLBuffer::Upload(const void* data, size_t size, size_t offset)
{
  glBindBuffer(m_Target, m_Buffer);
  glBufferSubData(m_Target,
                  static_cast<GLintptr>(offset),
                  static_cast<GLsizeiptr>(size),
                  data);
  glBindBuffer(m_Target, 0);
}

void OpenGLBuffer::Unmap()
{
  if (!m_Mapped) {
    return;
  }

  glBindBuffer(m_Target, m_Buffer);
  glUnmapBuffer(m_Target);
  glBindBuffer(m_Target, 0);
  m_Mapped = false;
}

auto OpenGLBuffer::GetSize() const -> size_t
{
  return m_Size;
}
