#ifndef RENDERER_RHI_OPENGL_OPENGLSWAPCHAIN_HPP
#define RENDERER_RHI_OPENGL_OPENGLSWAPCHAIN_HPP

#include "Renderer/RHI/RHISwapchain.hpp"

struct GLFWwindow;

class OpenGLSwapchain final : public RHISwapchain
{
public:
  OpenGLSwapchain(const OpenGLSwapchain&) = delete;
  OpenGLSwapchain(OpenGLSwapchain&&) = delete;
  auto operator=(const OpenGLSwapchain&) -> OpenGLSwapchain& = delete;
  auto operator=(OpenGLSwapchain&&) -> OpenGLSwapchain& = delete;
  explicit OpenGLSwapchain(GLFWwindow* window);
  ~OpenGLSwapchain() override = default;

  void Resize(uint32_t width, uint32_t height) override;

private:
  GLFWwindow* m_Window {nullptr};
};

#endif
