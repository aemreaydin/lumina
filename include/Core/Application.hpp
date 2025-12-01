#ifndef CORE_APPLICATION_HPP
#define CORE_APPLICATION_HPP

#include <memory>

#include "Window.hpp"

class Application
{
public:
  Application();
  Application(const Application&) = delete;
  Application(Application&&) = delete;
  auto operator=(const Application&) -> Application& = delete;
  auto operator=(Application&&) -> Application& = delete;
  virtual ~Application() = default;

  void Run();

  void OnEvent(void* event);

private:
  std::unique_ptr<Window> m_Window;
  bool m_Running = true;
  float m_LastFrameTime = 0.0F;
};

auto CreateApplication() -> std::unique_ptr<Application>;

#endif
