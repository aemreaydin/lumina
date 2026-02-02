#include <algorithm>
#include <stdexcept>

#include "UI/RHIImGui.hpp"

#include <imgui.h>

#include "Core/Logger.hpp"
#include "Renderer/RHI/OpenGL/OpenGLDevice.hpp"
#include "Renderer/RHI/OpenGL/OpenGLImGui.hpp"
#include "Renderer/RHI/Vulkan/VulkanDevice.hpp"
#include "Renderer/RHI/Vulkan/VulkanImGui.hpp"
#include "UI/ImGuiStyle.hpp"
#include "UI/SceneHierarchyPanel.hpp"
#include "UI/SettingsPanel.hpp"

auto RHIImGui::Create(RHIDevice& device) -> std::unique_ptr<RHIImGui>
{
  // Use dynamic_cast to determine the device type
  if (auto* vulkan_device = dynamic_cast<VulkanDevice*>(&device)) {
    return std::make_unique<VulkanImGui>(*vulkan_device);
  }

  if (auto* opengl_device = dynamic_cast<OpenGLDevice*>(&device)) {
    return std::make_unique<OpenGLImGui>(*opengl_device);
  }

  throw std::runtime_error("Unsupported RHI device type for ImGui");
}

namespace
{

constexpr float ANIM_SPEED = 8.0F;
constexpr float PANEL_WIDTH = 300.0F;

}  // namespace

RHIImGui::RHIImGui()
    : m_SettingsPanel(std::make_unique<SettingsPanel>())
    , m_SceneHierarchyPanel(std::make_unique<SceneHierarchyPanel>())
{
}

RHIImGui::~RHIImGui() = default;

void RHIImGui::RenderPanels(Scene& scene)
{
  const ImGuiIO& imgui_io = ImGui::GetIO();
  const float delta_time = imgui_io.DeltaTime;

  // Panel keybinds
  if (ImGui::IsKeyPressed(ImGuiKey_F1)) {
    ToggleSettings();
  }
  if (ImGui::IsKeyPressed(ImGuiKey_F2)) {
    ToggleSceneHierarchy();
  }

  // Animate settings panel
  const float settings_target = m_ShowSettings ? 1.0F : 0.0F;
  if (m_SettingsAnimProgress < settings_target) {
    m_SettingsAnimProgress =
        std::min(m_SettingsAnimProgress + ANIM_SPEED * delta_time, 1.0F);
  } else if (m_SettingsAnimProgress > settings_target) {
    m_SettingsAnimProgress =
        std::max(m_SettingsAnimProgress - ANIM_SPEED * delta_time, 0.0F);
  }

  // Animate hierarchy panel
  const float hierarchy_target = m_ShowSceneHierarchy ? 1.0F : 0.0F;
  if (m_HierarchyAnimProgress < hierarchy_target) {
    m_HierarchyAnimProgress =
        std::min(m_HierarchyAnimProgress + ANIM_SPEED * delta_time, 1.0F);
  } else if (m_HierarchyAnimProgress > hierarchy_target) {
    m_HierarchyAnimProgress =
        std::max(m_HierarchyAnimProgress - ANIM_SPEED * delta_time, 0.0F);
  }

  const ImVec2 display_size = imgui_io.DisplaySize;

  // Render settings panel (slides from right)
  if (m_SettingsAnimProgress > 0.0F) {
    const float offset = PANEL_WIDTH * (1.0F - m_SettingsAnimProgress);
    ImGui::SetNextWindowPos(
        ImVec2(display_size.x - PANEL_WIDTH + offset, 0.0F));
    ImGui::SetNextWindowSize(ImVec2(PANEL_WIDTH, display_size.y));

    m_SettingsPanel->Render(m_SettingsAnimProgress);
  }

  // Render scene hierarchy panel (slides from left)
  if (m_HierarchyAnimProgress > 0.0F) {
    const float offset = PANEL_WIDTH * (1.0F - m_HierarchyAnimProgress);
    ImGui::SetNextWindowPos(ImVec2(-offset, 0.0F));
    ImGui::SetNextWindowSize(ImVec2(PANEL_WIDTH, display_size.y));

    m_SceneHierarchyPanel->Render(scene, m_HierarchyAnimProgress);
  }

  // Render toggle buttons on screen edges
  renderToggleButtons(display_size);
}

void RHIImGui::renderToggleButtons(const ImVec2& display_size)
{
  constexpr float BUTTON_SIZE = 24.0F;
  constexpr float BUTTON_MARGIN = 8.0F;

  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0F, 0.0F));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(1.0F, 1.0F));

  // Left edge button (Scene Hierarchy)
  if (m_HierarchyAnimProgress < 0.1F) {
    ImGui::SetNextWindowPos(
        ImVec2(BUTTON_MARGIN, display_size.y * 0.5F - BUTTON_SIZE * 0.5F));
    ImGui::SetNextWindowSize(ImVec2(BUTTON_SIZE, BUTTON_SIZE));
    ImGui::Begin("##HierarchyToggle",
                 nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize
                     | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar
                     | ImGuiWindowFlags_NoBackground);
    if (ImGui::Button(">", ImVec2(BUTTON_SIZE, BUTTON_SIZE))) {
      ToggleSceneHierarchy();
    }
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("Scene Hierarchy (F2)");
    }
    ImGui::End();
  }

  // Right edge button (Settings)
  if (m_SettingsAnimProgress < 0.1F) {
    ImGui::SetNextWindowPos(ImVec2(display_size.x - BUTTON_SIZE - BUTTON_MARGIN,
                                   display_size.y * 0.5F - BUTTON_SIZE * 0.5F));
    ImGui::SetNextWindowSize(ImVec2(BUTTON_SIZE, BUTTON_SIZE));
    ImGui::Begin("##SettingsToggle",
                 nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize
                     | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar
                     | ImGuiWindowFlags_NoBackground);
    if (ImGui::Button("<", ImVec2(BUTTON_SIZE, BUTTON_SIZE))) {
      ToggleSettings();
    }
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("Settings (F1)");
    }
    ImGui::End();
  }

  ImGui::PopStyleVar(2);
}

void RHIImGui::ToggleSettings()
{
  m_ShowSettings = !m_ShowSettings;
  Logger::Trace("Settings panel: {}", m_ShowSettings ? "shown" : "hidden");
}

void RHIImGui::ToggleSceneHierarchy()
{
  m_ShowSceneHierarchy = !m_ShowSceneHierarchy;
  Logger::Trace("Scene hierarchy panel: {}",
                m_ShowSceneHierarchy ? "shown" : "hidden");
}

void RHIImGui::SetCamera(Camera& camera)
{
  m_SettingsPanel->SetCamera(&camera);
}

void RHIImGui::SetSelectedNode(SceneNode* node)
{
  m_SceneHierarchyPanel->SetSelectedNode(node);
}

void RHIImGui::SetCurrentAPI(RenderAPI api)
{
  m_SettingsPanel->SetCurrentAPI(api);
}

auto RHIImGui::GetPendingBackendSwitch() -> std::optional<RenderAPI>
{
  return m_SettingsPanel->GetPendingBackendSwitch();
}

auto RHIImGui::IsSettingsVisible() const -> bool
{
  return m_ShowSettings;
}

auto RHIImGui::IsSceneHierarchyVisible() const -> bool
{
  return m_ShowSceneHierarchy;
}
