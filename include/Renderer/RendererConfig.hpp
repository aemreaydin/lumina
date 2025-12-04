#ifndef RENDERER_RENDERERCONFIG_HPP
#define RENDERER_RENDERERCONFIG_HPP

#include <cstdint>

enum class RenderAPI : uint8_t
{
  OpenGL,
  Vulkan
};

struct RendererConfig
{
  RenderAPI API = RenderAPI::OpenGL;
  bool EnableValidation = true;
};

#endif
