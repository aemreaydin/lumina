#ifndef RENDERER_RHI_OPENGL_OPENGLIMGUI_HPP
#define RENDERER_RHI_OPENGL_OPENGLIMGUI_HPP

#include "UI/RHIImGui.hpp"

class OpenGLDevice;

/**
 * @brief OpenGL backend implementation for ImGui rendering.
 *
 * Uses ImGui's OpenGL3 backend with SDL3.
 */
class OpenGLImGui final : public RHIImGui
{
public:
  explicit OpenGLImGui(OpenGLDevice& device);
  ~OpenGLImGui() override = default;

  OpenGLImGui(const OpenGLImGui&) = delete;
  OpenGLImGui(OpenGLImGui&&) = delete;
  auto operator=(const OpenGLImGui&) -> OpenGLImGui& = delete;
  auto operator=(OpenGLImGui&&) -> OpenGLImGui& = delete;

  void Init(Window& window) override;
  void Shutdown() override;
  void BeginFrame() override;
  void EndFrame() override;

private:
  OpenGLDevice& m_Device;
};

#endif
