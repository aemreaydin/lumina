#ifndef CORE_APPLICATION_HPP
#define CORE_APPLICATION_HPP

#include <memory>

#include "Renderer/RendererConfig.hpp"
#include "Window.hpp"

class RHIDevice;

class Application
{
public:
  Application();
  Application(const Application&) = delete;
  Application(Application&&) = delete;
  auto operator=(const Application&) -> Application& = delete;
  auto operator=(Application&&) -> Application& = delete;
  virtual ~Application();

  void Run();

  void OnEvent(void* event);

private:
  void init_glfw() const;

  RendererConfig m_RendererConfig;
  std::unique_ptr<Window> m_Window;
  std::unique_ptr<RHIDevice> m_RHIDevice;
  bool m_Running = true;
  float m_LastFrameTime = 0.0F;
};

auto CreateApplication() -> std::unique_ptr<Application>;

#endif
