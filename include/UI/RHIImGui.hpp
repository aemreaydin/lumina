#ifndef UI_RHIIMGUI_HPP
#define UI_RHIIMGUI_HPP

#include <memory>
#include <optional>

#include "Renderer/RendererConfig.hpp"

struct ImVec2;

class Window;
class Scene;
class SceneNode;
class RHIDevice;
class Camera;
class SettingsPanel;
class SceneHierarchyPanel;

class RHIImGui
{
public:
  static auto Create(RHIDevice& device) -> std::unique_ptr<RHIImGui>;

  virtual void Init(Window& window) = 0;
  virtual void Shutdown() = 0;
  virtual void BeginFrame() = 0;
  virtual void EndFrame() = 0;

  void RenderPanels(Scene& scene);
  void ToggleSettings();
  void ToggleSceneHierarchy();

  void SetCamera(Camera& camera);
  void SetSelectedNode(SceneNode* node);
  void SetCurrentAPI(RenderAPI api);
  auto GetPendingBackendSwitch() -> std::optional<RenderAPI>;

  [[nodiscard]] auto IsSettingsVisible() const -> bool;
  [[nodiscard]] auto IsSceneHierarchyVisible() const -> bool;

  RHIImGui(const RHIImGui&) = delete;
  RHIImGui(RHIImGui&&) = delete;
  auto operator=(const RHIImGui&) -> RHIImGui& = delete;
  auto operator=(RHIImGui&&) -> RHIImGui& = delete;
  virtual ~RHIImGui();

protected:
  RHIImGui();

  void renderToggleButtons(const ImVec2& display_size);

  std::unique_ptr<SettingsPanel> m_SettingsPanel;
  std::unique_ptr<SceneHierarchyPanel> m_SceneHierarchyPanel;
  bool m_ShowSettings = false;
  bool m_ShowSceneHierarchy = false;
  float m_SettingsAnimProgress = 0.0F;
  float m_HierarchyAnimProgress = 0.0F;
};

#endif
