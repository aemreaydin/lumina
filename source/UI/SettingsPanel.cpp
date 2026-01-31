#include "UI/SettingsPanel.hpp"

#include <imgui.h>

#include "Renderer/Camera.hpp"

void SettingsPanel::Render(float animation_progress)
{
  if (animation_progress <= 0.0F) {
    return;
  }

  ImGui::PushStyleVar(ImGuiStyleVar_Alpha, animation_progress);

  constexpr ImGuiWindowFlags WINDOW_FLAGS = ImGuiWindowFlags_NoTitleBar
      | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove
      | ImGuiWindowFlags_NoCollapse;

  if (ImGui::Begin("Settings", nullptr, WINDOW_FLAGS)) {
    ImGui::Text("SETTINGS");
    ImGui::Separator();

    renderRendererSection();
    renderCameraSection();
    renderDebugSection();
  }
  ImGui::End();

  ImGui::PopStyleVar();
}

void SettingsPanel::UpdateStats(const PerformanceStats& stats)
{
  m_Stats = stats;
}

void SettingsPanel::SetDevice(RHIDevice* device)
{
  m_Device = device;
}

void SettingsPanel::SetCamera(Camera* camera)
{
  m_Camera = camera;
}

void SettingsPanel::SetAPIName(const std::string& api_name)
{
  m_APIName = api_name;
}

void SettingsPanel::SetValidationEnabled(bool enabled)
{
  m_ValidationEnabled = enabled;
}

void SettingsPanel::SetResolution(uint32_t width, uint32_t height)
{
  m_Width = width;
  m_Height = height;
}

void SettingsPanel::renderRendererSection()
{
  if (ImGui::CollapsingHeader("Renderer", ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::Text("API: %s", m_APIName.c_str());
    ImGui::Text("Validation: %s", m_ValidationEnabled ? "Enabled" : "Disabled");

    bool vsync = true;
    if (ImGui::Checkbox("VSync", &vsync)) {
      // TODO: Apply vsync change
    }

    ImGui::Text("Resolution: %u x %u", m_Width, m_Height);
  }
}

void SettingsPanel::renderCameraSection()
{
  if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
    if (m_Camera == nullptr) {
      ImGui::TextDisabled("No camera set");
      return;
    }

    float fov = m_Camera->GetFOV();
    if (ImGui::SliderFloat("FOV", &fov, 30.0F, 120.0F, "%.1f")) {
      // TODO: Apply FOV change - need setter in Camera
    }

    float near_plane = m_Camera->GetNearPlane();
    float far_plane = m_Camera->GetFarPlane();

    if (ImGui::DragFloat("Near Plane", &near_plane, 0.01F, 0.001F, 10.0F)) {
      // TODO: Apply near plane change
    }
    if (ImGui::DragFloat("Far Plane", &far_plane, 1.0F, 10.0F, 10000.0F)) {
      // TODO: Apply far plane change
    }

    static float move_speed = 10.0F;
    static float sensitivity = 0.3F;
    ImGui::SliderFloat("Move Speed", &move_speed, 1.0F, 50.0F);
    ImGui::SliderFloat("Sensitivity", &sensitivity, 0.1F, 1.0F);
  }
}

void SettingsPanel::renderDebugSection()
{
  if (ImGui::CollapsingHeader("Debug", ImGuiTreeNodeFlags_DefaultOpen)) {
    // Performance stats
    ImGui::Text("Performance");
    ImGui::Indent();
    ImGui::Text("FPS: %.1f", m_Stats.Fps);
    ImGui::Text("Frame Time: %.2f ms", m_Stats.FrameTime * 1000.0F);
    ImGui::Text("Draw Calls: %u", m_Stats.DrawCalls);
    ImGui::Text("Memory: %llu MB",
                static_cast<unsigned long long>(m_Stats.MemoryUsageMb));
    ImGui::Unindent();

    ImGui::Separator();

    // Debug visualization toggles
    ImGui::Text("Visualization");
    ImGui::Indent();
    if (ImGui::Checkbox("Wireframe", &m_Wireframe)) {
      // TODO: Apply wireframe mode
    }
    if (ImGui::Checkbox("Bounding Boxes", &m_ShowBoundingBoxes)) {
      // TODO: Apply bounding box visualization
    }
    if (ImGui::Checkbox("Normals", &m_ShowNormals)) {
      // TODO: Apply normal visualization
    }
    ImGui::Unindent();
  }
}
