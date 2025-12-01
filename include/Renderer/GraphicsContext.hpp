#ifndef RENDERER_GRAPHICS_CONTEXT_HPP
#define RENDERER_GRAPHICS_CONTEXT_HPP

class GraphicsContext
{
public:
  GraphicsContext() = default;
  GraphicsContext(const GraphicsContext&) = delete;
  GraphicsContext(GraphicsContext&&) = delete;
  auto operator=(const GraphicsContext&) -> GraphicsContext& = delete;
  auto operator=(GraphicsContext&&) -> GraphicsContext& = delete;
  virtual ~GraphicsContext() = default;

  virtual void SetGLFWWindow(struct GLFWwindow* window) = 0;

  virtual void Init() = 0;
  virtual void LoadFns() = 0;
  virtual void SwapBuffers() = 0;
};

#endif
