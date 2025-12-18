#include "Renderer/RHI/OpenGL/OpenGLDescriptorSet.hpp"

#include <glad/glad.h>

#include "Core/Logger.hpp"
#include "Renderer/RHI/OpenGL/OpenGLBuffer.hpp"
#include "Renderer/RHI/OpenGL/OpenGLSampler.hpp"
#include "Renderer/RHI/OpenGL/OpenGLTexture.hpp"

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

void OpenGLDescriptorSet::WriteCombinedImageSampler(uint32_t binding,
                                                    RHITexture* texture,
                                                    RHISampler* sampler)
{
  const auto* gl_texture = dynamic_cast<const OpenGLTexture*>(texture);
  const auto* gl_sampler = dynamic_cast<const OpenGLSampler*>(sampler);

  if (gl_texture == nullptr || gl_sampler == nullptr) {
    Logger::Error("[OpenGL] WriteCombinedImageSampler: Invalid texture or sampler");
    return;
  }

  m_TextureBindings[binding] =
      OpenGLTextureBinding {.Texture = gl_texture, .Sampler = gl_sampler};

  Logger::Trace("[OpenGL] Descriptor set: wrote texture/sampler to binding {}",
                binding);
}

void OpenGLDescriptorSet::Bind() const
{
  // Bind uniform buffers
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

  // Bind textures and samplers
  for (const auto& [binding, texture_binding] : m_TextureBindings) {
    if (texture_binding.Texture == nullptr || texture_binding.Sampler == nullptr) {
      continue;
    }

    // Bind texture to texture unit (binding point)
    glBindTextureUnit(binding, texture_binding.Texture->GetGLTexture());
    // Bind sampler to the same texture unit
    glBindSampler(binding, texture_binding.Sampler->GetGLSampler());
  }
}
