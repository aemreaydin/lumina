#ifndef RENDERER_OPENGL_CONTEXT_HPP
#define RENDERER_OPENGL_CONTEXT_HPP

#include "GraphicsContext.hpp"

class OpenGLContext : public GraphicsContext
{
public:
  OpenGLContext() = default;
  OpenGLContext(const OpenGLContext&) = delete;
  OpenGLContext(OpenGLContext&&) = delete;
  auto operator=(const OpenGLContext&) -> OpenGLContext& = delete;
  auto operator=(OpenGLContext&&) -> OpenGLContext& = delete;
  ~OpenGLContext() override;

  void SetGLFWWindow(GLFWwindow* window) override;

  void Init() override;
  void LoadFns() override;
  void SwapBuffers() override;

private:
  GLFWwindow* m_Window {nullptr};
};

#endif
