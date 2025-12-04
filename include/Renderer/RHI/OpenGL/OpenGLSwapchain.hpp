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

  auto AcquireNextImage() -> uint32_t override;
  void Present(uint32_t image_index) override;
  void Resize(uint32_t width, uint32_t height) override;

private:
  GLFWwindow* m_Window {nullptr};
};

#endif
