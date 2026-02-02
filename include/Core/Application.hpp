#ifndef CORE_APPLICATION_HPP
#define CORE_APPLICATION_HPP

#include <memory>

#include "Core/PerformanceStats.hpp"
#include "Renderer/RendererConfig.hpp"
#include "Window.hpp"

class RHIDevice;
class RHIImGui;

class Application
{
public:
  Application();
  Application(const Application&) = delete;
  Application(Application&&) = delete;
  auto operator=(const Application&) -> Application& = delete;
  auto operator=(Application&&) -> Application& = delete;
  virtual ~Application();

  void Init();
  void Destroy();
  void Run();

  static void OnEvent(void* event);

protected:
  virtual void OnUpdate([[maybe_unused]] float delta_time) {}

  virtual void OnRender([[maybe_unused]] float delta_time) {}

  virtual void OnInit() {}

  virtual void OnDestroy() {}

  [[nodiscard]] auto GetDevice() -> RHIDevice& { return *m_RHIDevice; }

  [[nodiscard]] auto GetDevice() const -> const RHIDevice&
  {
    return *m_RHIDevice;
  }

  [[nodiscard]] auto GetWindow() -> Window& { return *m_Window; }

  [[nodiscard]] auto GetImGui() -> RHIImGui& { return *m_ImGui; }

  [[nodiscard]] auto GetImGui() const -> const RHIImGui& { return *m_ImGui; }

  [[nodiscard]] auto GetRendererConfig() const -> const RendererConfig&
  {
    return m_RendererConfig;
  }

  [[nodiscard]] auto GetStats() const -> const PerformanceStats&
  {
    return m_PerfTracker.GetStats();
  }

  void SwitchBackend(RenderAPI new_api);

private:
  static void InitSdl();

  RendererConfig m_RendererConfig;
  std::unique_ptr<Window> m_Window;
  std::unique_ptr<RHIDevice> m_RHIDevice;
  std::unique_ptr<RHIImGui> m_ImGui;
  bool m_Running = true;
  uint64_t m_StartTime {0};
  uint64_t m_LastFrameTime {0};
  PerformanceTracker m_PerfTracker;
};

auto CreateApplication() -> std::unique_ptr<Application>;

#endif
