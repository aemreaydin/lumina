#ifndef CORE_APPLICATION_HPP
#define CORE_APPLICATION_HPP

#include <memory>

#include "Renderer/RendererConfig.hpp"
#include "Window.hpp"

class RHIDevice;

class Application
{
public:
  Application() = default;
  Application(const Application&) = delete;
  Application(Application&&) = delete;
  auto operator=(const Application&) -> Application& = delete;
  auto operator=(Application&&) -> Application& = delete;
  virtual ~Application() = default;

  void Init();
  void Destroy();
  void Run();

  void OnEvent(void* event);

protected:
  virtual void OnRender([[maybe_unused]] float delta_time) {}

  virtual void OnInit() {}

  virtual void OnDestroy() {}

  [[nodiscard]] auto GetDevice() -> RHIDevice& { return *m_RHIDevice; }

  [[nodiscard]] auto GetDevice() const -> const RHIDevice&
  {
    return *m_RHIDevice;
  }

private:
  static void InitSdl();

  RendererConfig m_RendererConfig;
  std::unique_ptr<Window> m_Window;
  std::unique_ptr<RHIDevice> m_RHIDevice;
  bool m_Running = true;
  uint64_t m_StartTime {0};
  uint64_t m_LastFrameTime {0};
};

auto CreateApplication() -> std::unique_ptr<Application>;

#endif
