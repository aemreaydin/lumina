#ifndef RENDERER_RHI_OPENGL_OPENGLSWAPCHAIN_HPP
#define RENDERER_RHI_OPENGL_OPENGLSWAPCHAIN_HPP

#include <SDL3/SDL.h>

#include "Renderer/RHI/RHISwapchain.hpp"

class OpenGLSwapchain final : public RHISwapchain
{
public:
  OpenGLSwapchain(const OpenGLSwapchain&) = delete;
  OpenGLSwapchain(OpenGLSwapchain&&) = delete;
  auto operator=(const OpenGLSwapchain&) -> OpenGLSwapchain& = delete;
  auto operator=(OpenGLSwapchain&&) -> OpenGLSwapchain& = delete;
  explicit OpenGLSwapchain(SDL_Window* window);
  ~OpenGLSwapchain() override = default;

  void Resize(uint32_t width, uint32_t height) override;

private:
  SDL_Window* m_Window {nullptr};
};

#endif
