#ifndef RENDERER_RHI_OPENGL_OPENGLDEVICE_HPP
#define RENDERER_RHI_OPENGL_OPENGLDEVICE_HPP

#include <memory>

#include "Renderer/RHI/OpenGL/OpenGLSwapchain.hpp"
#include "Renderer/RHI/RHIDevice.hpp"

struct RendererConfig;

class OpenGLDevice final : public RHIDevice
{
public:
  OpenGLDevice() = default;
  OpenGLDevice(const OpenGLDevice&) = delete;
  OpenGLDevice(OpenGLDevice&&) = delete;
  auto operator=(const OpenGLDevice&) -> OpenGLDevice& = delete;
  auto operator=(OpenGLDevice&&) -> OpenGLDevice& = delete;
  ~OpenGLDevice() override = default;

  void Init(const RendererConfig& config, void* window) override;
  void CreateSwapchain(uint32_t width, uint32_t height) override;
  void Destroy() override;

  void BeginFrame() override;
  void EndFrame() override;
  void Present() override;

  [[nodiscard]] auto GetSwapchain() -> RHISwapchain* override;

private:
  std::unique_ptr<OpenGLSwapchain> m_Swapchain;
  GLFWwindow* m_Window {nullptr};
  bool m_Initialized {false};
};

#endif
