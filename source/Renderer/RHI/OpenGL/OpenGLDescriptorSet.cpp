#include "Renderer/RHI/OpenGL/OpenGLDescriptorSet.hpp"

#include <glad/glad.h>

#include "Core/Logger.hpp"
#include "Renderer/RHI/OpenGL/OpenGLBuffer.hpp"

OpenGLDescriptorSetLayout::OpenGLDescriptorSetLayout(
    const DescriptorSetLayoutDesc& desc)
    : m_Bindings(desc.Bindings)
{
  Logger::Trace("[OpenGL] Created descriptor set layout with {} bindings",
                m_Bindings.size());
}

auto OpenGLDescriptorSetLayout::GetBindings() const
    -> const std::vector<DescriptorBinding>&
{
  return m_Bindings;
}

OpenGLDescriptorSet::OpenGLDescriptorSet(
    const std::shared_ptr<RHIDescriptorSetLayout>& layout)
    : m_Layout(layout)
{
  Logger::Trace("[OpenGL] Created descriptor set");
}

void OpenGLDescriptorSet::WriteBuffer(uint32_t binding,
                                      RHIBuffer* buffer,
                                      size_t offset,
                                      size_t range)
{
  const auto* gl_buffer = dynamic_cast<const OpenGLBuffer*>(buffer);
  if (gl_buffer == nullptr) {
    Logger::Error("[OpenGL] WriteBuffer: Invalid buffer");
    return;
  }

  m_BufferBindings[binding] =
      OpenGLBufferBinding {.Buffer = gl_buffer,
                           .Offset = offset,
                           .Range = range == 0 ? gl_buffer->GetSize() : range};

  Logger::Trace("[OpenGL] Descriptor set: wrote buffer to binding {}", binding);
}

void OpenGLDescriptorSet::Bind() const
{
  for (const auto& [binding, buffer_binding] : m_BufferBindings) {
    if (buffer_binding.Buffer == nullptr) {
      continue;
    }

    glBindBufferRange(GL_UNIFORM_BUFFER,
                      binding,
                      buffer_binding.Buffer->GetGLBuffer(),
                      static_cast<GLintptr>(buffer_binding.Offset),
                      static_cast<GLsizeiptr>(buffer_binding.Range));
  }
}
