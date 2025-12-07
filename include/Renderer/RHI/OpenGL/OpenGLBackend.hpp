#ifndef RENDERER_RHI_OPENGL_OPENGLBACKEND_HPP
#define RENDERER_RHI_OPENGL_OPENGLBACKEND_HPP

#include <cstdint>

struct OpenGLBackend
{
  using CommandBufferHandle = void*;
  using PipelineHandle = uint32_t;  // VAO
  using BufferHandle = uint32_t;  // VBO
};

#endif
