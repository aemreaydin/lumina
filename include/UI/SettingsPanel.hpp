#ifndef UI_SETTINGSPANEL_HPP
#define UI_SETTINGSPANEL_HPP

#include <cstdint>
#include <optional>
#include <string>

#include "Renderer/RendererConfig.hpp"

class RHIDevice;
class Camera;

struct PerformanceStats
{
  float FrameTime = 0.0F;
  float Fps = 0.0F;
  uint32_t DrawCalls = 0;
  uint64_t MemoryUsageMb = 0;
};

class SettingsPanel
{
public:
  void Render(float animation_progress);
  void UpdateStats(const PerformanceStats& stats);

  void SetDevice(RHIDevice* device);
  void SetCamera(Camera* camera);
  void SetAPIName(const std::string& api_name);
  void SetValidationEnabled(bool enabled);
  void SetResolution(uint32_t width, uint32_t height);

  void SetCurrentAPI(RenderAPI api);
  auto GetPendingBackendSwitch() -> std::optional<RenderAPI>;

private:
  void renderRendererSection();
  void renderCameraSection();
  void renderDebugSection();

  RHIDevice* m_Device = nullptr;
  Camera* m_Camera = nullptr;
  PerformanceStats m_Stats {};

  std::string m_APIName = "Unknown";
  bool m_ValidationEnabled = false;
  uint32_t m_Width = 0;
  uint32_t m_Height = 0;

  // Backend switch
  RenderAPI m_CurrentAPI = RenderAPI::OpenGL;
  std::optional<RenderAPI> m_PendingBackendSwitch;

  // Debug toggles
  bool m_Wireframe = false;
  bool m_ShowBoundingBoxes = false;
  bool m_ShowNormals = false;
};

#endif
