# ImGui UI System Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Add an RHI-integrated ImGui UI system with collapsible sidebars for settings/debug info and scene hierarchy with drag-drop reparenting.

**Architecture:** Backend-specific ImGui implementations (VulkanImGui, OpenGLImGui) behind an abstract RHIImGui interface. Collapsible sidebars slide in from screen edges. Flat minimalist styling inspired by Blender 3.0+.

**Tech Stack:** Dear ImGui with SDL3 and Vulkan/OpenGL3 backends, integrated via vcpkg.

---

## Task 1: Add ImGui Dependency

**Files:**
- Modify: `vcpkg.json`

**Step 1: Add imgui to vcpkg.json**

Open `vcpkg.json` and add imgui with the required backend features. Add after the `tinyobjloader` entry:

```json
{
  "name": "imgui",
  "version>=": "1.91.0",
  "features": ["sdl3-binding", "vulkan-binding", "opengl3-binding", "docking-experimental"]
}
```

The full dependencies array should now include imgui at the end.

**Step 2: Update vcpkg and verify installation**

Run: `cmake --preset=simple-win64`

Expected: CMake configures successfully and vcpkg installs imgui with SDL3, Vulkan, and OpenGL3 bindings.

**Step 3: Commit**

```bash
git add vcpkg.json
git commit -m "$(cat <<'EOF'
deps: add imgui with SDL3, Vulkan, and OpenGL3 bindings

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>
EOF
)"
```

---

## Task 2: Add ImGui to CMakeLists.txt

**Files:**
- Modify: `CMakeLists.txt:130-135`

**Step 1: Add find_package and link imgui**

After the slang link (line 134), add:

```cmake
find_package(imgui CONFIG REQUIRED)
target_link_libraries(lumina_lumina PUBLIC imgui::imgui)
```

**Step 2: Verify build still compiles**

Run: `task build`

Expected: Build succeeds with no errors.

**Step 3: Commit**

```bash
git add CMakeLists.txt
git commit -m "$(cat <<'EOF'
build: link imgui library to lumina

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>
EOF
)"
```

---

## Task 3: Create ImGuiStyle Header and Implementation

**Files:**
- Create: `include/UI/ImGuiStyle.hpp`
- Create: `source/UI/ImGuiStyle.cpp`

**Step 1: Create the header file**

Create `include/UI/ImGuiStyle.hpp`:

```cpp
#ifndef UI_IMGUISTYLE_HPP
#define UI_IMGUISTYLE_HPP

namespace ImGuiStyle
{

void ApplyFlatTheme();

} // namespace ImGuiStyle

#endif
```

**Step 2: Create the implementation file**

Create `source/UI/ImGuiStyle.cpp`:

```cpp
#include "UI/ImGuiStyle.hpp"

#include <imgui.h>

namespace ImGuiStyle
{

void ApplyFlatTheme()
{
    ImGuiStyle& style = ImGui::GetStyle();

    // Flat minimalist - no rounding, minimal borders
    style.WindowRounding = 0.0F;
    style.ChildRounding = 0.0F;
    style.FrameRounding = 2.0F;
    style.PopupRounding = 0.0F;
    style.ScrollbarRounding = 0.0F;
    style.GrabRounding = 2.0F;
    style.TabRounding = 0.0F;

    // Compact padding
    style.WindowPadding = ImVec2(8.0F, 8.0F);
    style.FramePadding = ImVec2(4.0F, 3.0F);
    style.ItemSpacing = ImVec2(8.0F, 4.0F);
    style.ItemInnerSpacing = ImVec2(4.0F, 4.0F);

    // Borders
    style.WindowBorderSize = 1.0F;
    style.ChildBorderSize = 1.0F;
    style.FrameBorderSize = 1.0F;
    style.PopupBorderSize = 1.0F;

    // Colors - Blender/Unity inspired dark theme
    ImVec4* colors = style.Colors;

    // Backgrounds
    colors[ImGuiCol_WindowBg] = ImVec4(0.145F, 0.145F, 0.145F, 1.0F);        // #252525
    colors[ImGuiCol_ChildBg] = ImVec4(0.114F, 0.114F, 0.114F, 1.0F);         // #1D1D1D
    colors[ImGuiCol_PopupBg] = ImVec4(0.145F, 0.145F, 0.145F, 1.0F);

    // Borders
    colors[ImGuiCol_Border] = ImVec4(0.25F, 0.25F, 0.25F, 1.0F);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.0F, 0.0F, 0.0F, 0.0F);

    // Frame backgrounds (inputs, sliders)
    colors[ImGuiCol_FrameBg] = ImVec4(0.2F, 0.2F, 0.2F, 1.0F);               // #333333
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.267F, 0.267F, 0.267F, 1.0F);  // #444444
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.314F, 0.314F, 0.314F, 1.0F);   // #505050

    // Title bar
    colors[ImGuiCol_TitleBg] = ImVec4(0.114F, 0.114F, 0.114F, 1.0F);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.145F, 0.145F, 0.145F, 1.0F);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.114F, 0.114F, 0.114F, 0.5F);

    // Menu bar
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.145F, 0.145F, 0.145F, 1.0F);

    // Scrollbar
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.114F, 0.114F, 0.114F, 1.0F);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.3F, 0.3F, 0.3F, 1.0F);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.4F, 0.4F, 0.4F, 1.0F);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.5F, 0.5F, 0.5F, 1.0F);

    // Buttons
    colors[ImGuiCol_Button] = ImVec4(0.2F, 0.2F, 0.2F, 1.0F);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.267F, 0.267F, 0.267F, 1.0F);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.314F, 0.314F, 0.314F, 1.0F);

    // Headers (collapsing headers, tree nodes)
    colors[ImGuiCol_Header] = ImVec4(0.2F, 0.2F, 0.2F, 1.0F);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.29F, 0.435F, 0.647F, 0.8F);    // Accent blue
    colors[ImGuiCol_HeaderActive] = ImVec4(0.29F, 0.435F, 0.647F, 1.0F);     // #4A6FA5

    // Tabs
    colors[ImGuiCol_Tab] = ImVec4(0.145F, 0.145F, 0.145F, 1.0F);
    colors[ImGuiCol_TabHovered] = ImVec4(0.29F, 0.435F, 0.647F, 0.8F);
    colors[ImGuiCol_TabActive] = ImVec4(0.2F, 0.2F, 0.2F, 1.0F);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.114F, 0.114F, 0.114F, 1.0F);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.145F, 0.145F, 0.145F, 1.0F);

    // Text
    colors[ImGuiCol_Text] = ImVec4(0.878F, 0.878F, 0.878F, 1.0F);            // #E0E0E0
    colors[ImGuiCol_TextDisabled] = ImVec4(0.439F, 0.439F, 0.439F, 1.0F);    // #707070

    // Separators
    colors[ImGuiCol_Separator] = ImVec4(0.25F, 0.25F, 0.25F, 1.0F);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.29F, 0.435F, 0.647F, 0.8F);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.29F, 0.435F, 0.647F, 1.0F);

    // Resize grip
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.2F, 0.2F, 0.2F, 0.5F);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.29F, 0.435F, 0.647F, 0.8F);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.29F, 0.435F, 0.647F, 1.0F);

    // Checkmark, slider grab
    colors[ImGuiCol_CheckMark] = ImVec4(0.29F, 0.435F, 0.647F, 1.0F);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.29F, 0.435F, 0.647F, 0.8F);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.29F, 0.435F, 0.647F, 1.0F);

    // Drag-drop
    colors[ImGuiCol_DragDropTarget] = ImVec4(0.29F, 0.435F, 0.647F, 1.0F);
}

} // namespace ImGuiStyle
```

**Step 3: Add source file to CMakeLists.txt**

In `CMakeLists.txt`, add to the library sources (after Scene sources, around line 44):

```cmake
        # UI
        source/UI/ImGuiStyle.cpp
```

**Step 4: Verify build compiles**

Run: `task build`

Expected: Build succeeds with no errors.

**Step 5: Commit**

```bash
git add include/UI/ImGuiStyle.hpp source/UI/ImGuiStyle.cpp CMakeLists.txt
git commit -m "$(cat <<'EOF'
feat(ui): add flat minimalist ImGui theme

Blender 3.0+ / Unity inspired dark theme with:
- Muted gray backgrounds
- Subtle blue accent color (#4A6FA5)
- Minimal rounding and clean borders

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>
EOF
)"
```

---

## Task 4: Create RHIImGui Abstract Interface

**Files:**
- Create: `include/UI/RHIImGui.hpp`
- Create: `source/UI/RHIImGui.cpp`

**Step 1: Create the header file**

Create `include/UI/RHIImGui.hpp`:

```cpp
#ifndef UI_RHIIMGUI_HPP
#define UI_RHIIMGUI_HPP

#include <memory>

class Window;
class Scene;
class RHIDevice;
class Camera;
class SettingsPanel;
class SceneHierarchyPanel;

class RHIImGui
{
public:
    static auto Create(RHIDevice* device) -> std::unique_ptr<RHIImGui>;

    virtual void Init(Window* window) = 0;
    virtual void Shutdown() = 0;
    virtual void BeginFrame() = 0;
    virtual void EndFrame() = 0;

    void RenderPanels(Scene* scene);
    void ToggleSettings();
    void ToggleSceneHierarchy();
    void HandleInput(void* event);

    void SetCamera(Camera* camera);

    [[nodiscard]] auto IsSettingsVisible() const -> bool;
    [[nodiscard]] auto IsSceneHierarchyVisible() const -> bool;

    RHIImGui(const RHIImGui&) = delete;
    RHIImGui(RHIImGui&&) = delete;
    auto operator=(const RHIImGui&) -> RHIImGui& = delete;
    auto operator=(RHIImGui&&) -> RHIImGui& = delete;
    virtual ~RHIImGui();

protected:
    RHIImGui();

    std::unique_ptr<SettingsPanel> m_SettingsPanel;
    std::unique_ptr<SceneHierarchyPanel> m_SceneHierarchyPanel;
    bool m_ShowSettings = false;
    bool m_ShowSceneHierarchy = false;
    float m_SettingsAnimProgress = 0.0F;
    float m_HierarchyAnimProgress = 0.0F;
};

#endif
```

**Step 2: Create the implementation file**

Create `source/UI/RHIImGui.cpp`:

```cpp
#include "UI/RHIImGui.hpp"

#include <imgui.h>

#include "Core/Logger.hpp"
#include "Renderer/RendererConfig.hpp"
#include "UI/ImGuiStyle.hpp"
#include "UI/SceneHierarchyPanel.hpp"
#include "UI/SettingsPanel.hpp"

// Forward declarations for backend implementations
class VulkanImGui;
class OpenGLImGui;

namespace
{

constexpr float ANIM_SPEED = 8.0F;
constexpr float PANEL_WIDTH = 300.0F;

} // namespace

RHIImGui::RHIImGui()
    : m_SettingsPanel(std::make_unique<SettingsPanel>())
    , m_SceneHierarchyPanel(std::make_unique<SceneHierarchyPanel>())
{
}

RHIImGui::~RHIImGui() = default;

void RHIImGui::RenderPanels(Scene* scene)
{
    const ImGuiIO& imgui_io = ImGui::GetIO();
    const float delta_time = imgui_io.DeltaTime;

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
        ImGui::SetNextWindowPos(
            ImVec2(display_size.x - BUTTON_SIZE - BUTTON_MARGIN,
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

void RHIImGui::HandleInput(void* /*event*/)
{
    // SDL event processing is handled by backend-specific implementations
}

void RHIImGui::SetCamera(Camera* camera)
{
    m_SettingsPanel->SetCamera(camera);
}

auto RHIImGui::IsSettingsVisible() const -> bool
{
    return m_ShowSettings;
}

auto RHIImGui::IsSceneHierarchyVisible() const -> bool
{
    return m_ShowSceneHierarchy;
}
```

**Step 3: Add renderToggleButtons declaration to header**

Add to `include/UI/RHIImGui.hpp` in the protected section:

```cpp
protected:
    RHIImGui();

    void renderToggleButtons(const struct ImVec2& display_size);

    std::unique_ptr<SettingsPanel> m_SettingsPanel;
    // ... rest of members
```

**Step 4: Add source file to CMakeLists.txt**

In `CMakeLists.txt`, add after ImGuiStyle.cpp:

```cmake
        source/UI/RHIImGui.cpp
```

**Step 5: Commit (will complete after panel classes are added)**

Note: This won't compile yet - needs SettingsPanel and SceneHierarchyPanel. Continue to next tasks.

---

## Task 5: Create SettingsPanel

**Files:**
- Create: `include/UI/SettingsPanel.hpp`
- Create: `source/UI/SettingsPanel.cpp`

**Step 1: Create the header file**

Create `include/UI/SettingsPanel.hpp`:

```cpp
#ifndef UI_SETTINGSPANEL_HPP
#define UI_SETTINGSPANEL_HPP

#include <cstdint>

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

private:
    void renderRendererSection();
    void renderCameraSection();
    void renderDebugSection();

    RHIDevice* m_Device = nullptr;
    Camera* m_Camera = nullptr;
    PerformanceStats m_Stats {};

    // Debug toggles
    bool m_Wireframe = false;
    bool m_ShowBoundingBoxes = false;
    bool m_ShowNormals = false;
};

#endif
```

**Step 2: Create the implementation file**

Create `source/UI/SettingsPanel.cpp`:

```cpp
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

void SettingsPanel::renderRendererSection()
{
    if (ImGui::CollapsingHeader("Renderer", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("API: Vulkan");  // TODO: Get from device
        ImGui::Text("Validation: Enabled");  // TODO: Get from device

        bool vsync = true;
        if (ImGui::Checkbox("VSync", &vsync)) {
            // TODO: Apply vsync change
        }

        ImGui::Text("Resolution: 1280 x 720");  // TODO: Get from swapchain
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
        ImGui::Text("Memory: %llu MB", m_Stats.MemoryUsageMb);
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
```

**Step 3: Add Camera getters**

The Camera class needs FOV, near, and far plane getters. Check `include/Renderer/Camera.hpp` - they already exist (GetFOV is missing, but m_FOV is accessible).

Add to `include/Renderer/Camera.hpp` after line 46 (after GetYaw):

```cpp
  [[nodiscard]] auto GetFOV() const -> float { return m_FOV; }
  [[nodiscard]] auto GetNearPlane() const -> float { return m_NearPlane; }
  [[nodiscard]] auto GetFarPlane() const -> float { return m_FarPlane; }
```

**Step 4: Add source file to CMakeLists.txt**

In `CMakeLists.txt`, add after RHIImGui.cpp:

```cmake
        source/UI/SettingsPanel.cpp
```

**Step 5: Commit (after Task 6 - needs SceneHierarchyPanel first)**

---

## Task 6: Create SceneHierarchyPanel

**Files:**
- Create: `include/UI/SceneHierarchyPanel.hpp`
- Create: `source/UI/SceneHierarchyPanel.cpp`

**Step 1: Create the header file**

Create `include/UI/SceneHierarchyPanel.hpp`:

```cpp
#ifndef UI_SCENEHIERARCHYPANEL_HPP
#define UI_SCENEHIERARCHYPANEL_HPP

#include <functional>

class Scene;
class SceneNode;

class SceneHierarchyPanel
{
public:
    void Render(Scene* scene, float animation_progress);

    [[nodiscard]] auto GetSelectedNode() const -> SceneNode*;
    void SetSelectedNode(SceneNode* node);

    void SetOnSelectionChanged(std::function<void(SceneNode*)> callback);

private:
    void renderNode(SceneNode* node);
    void handleDragDrop(SceneNode* node);

    SceneNode* m_SelectedNode = nullptr;
    SceneNode* m_DraggedNode = nullptr;
    std::function<void(SceneNode*)> m_OnSelectionChanged;
};

#endif
```

**Step 2: Create the implementation file**

Create `source/UI/SceneHierarchyPanel.cpp`:

```cpp
#include "UI/SceneHierarchyPanel.hpp"

#include <imgui.h>

#include "Core/Logger.hpp"
#include "Renderer/Scene/Scene.hpp"
#include "Renderer/Scene/SceneNode.hpp"

void SceneHierarchyPanel::Render(Scene* scene, float animation_progress)
{
    if (animation_progress <= 0.0F || scene == nullptr) {
        return;
    }

    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, animation_progress);

    constexpr ImGuiWindowFlags WINDOW_FLAGS = ImGuiWindowFlags_NoTitleBar
        | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove
        | ImGuiWindowFlags_NoCollapse;

    if (ImGui::Begin("Scene Hierarchy", nullptr, WINDOW_FLAGS)) {
        ImGui::Text("SCENE HIERARCHY");
        ImGui::Separator();

        ImGui::Text("%s", scene->GetName().c_str());
        ImGui::Separator();

        // Render the root node's children (root itself is usually not shown)
        SceneNode& root = scene->GetRoot();
        for (const auto& child : root.GetChildren()) {
            renderNode(child.get());
        }

        // Handle drop on empty space (unparent to root)
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload =
                    ImGui::AcceptDragDropPayload("SCENE_NODE"))
            {
                auto* dropped_node = *static_cast<SceneNode**>(payload->Data);
                if (dropped_node != nullptr
                    && dropped_node->GetParent() != &root)
                {
                    Logger::Info("Reparenting '{}' to root",
                                 dropped_node->GetName());

                    // Remove from current parent and add to root
                    SceneNode* old_parent = dropped_node->GetParent();
                    if (old_parent != nullptr) {
                        // Find and move the unique_ptr
                        auto& siblings = const_cast<std::vector<std::unique_ptr<SceneNode>>&>(
                            old_parent->GetChildren());
                        for (auto it = siblings.begin(); it != siblings.end(); ++it) {
                            if (it->get() == dropped_node) {
                                auto node_ptr = std::move(*it);
                                siblings.erase(it);
                                root.AddChild(std::move(node_ptr));
                                break;
                            }
                        }
                    }
                }
            }
            ImGui::EndDragDropTarget();
        }
    }
    ImGui::End();

    ImGui::PopStyleVar();
}

void SceneHierarchyPanel::renderNode(SceneNode* node)
{
    if (node == nullptr) {
        return;
    }

    const bool has_children = node->GetChildCount() > 0;
    const bool is_selected = (m_SelectedNode == node);

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow
        | ImGuiTreeNodeFlags_OpenOnDoubleClick
        | ImGuiTreeNodeFlags_SpanAvailWidth;

    if (!has_children) {
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    }
    if (is_selected) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    const bool node_open = ImGui::TreeNodeEx(
        static_cast<void*>(node), flags, "%s", node->GetName().c_str());

    // Handle selection
    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
        SetSelectedNode(node);
    }

    // Drag source
    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
        m_DraggedNode = node;
        ImGui::SetDragDropPayload("SCENE_NODE", &node, sizeof(SceneNode*));
        ImGui::Text("Moving: %s", node->GetName().c_str());
        ImGui::EndDragDropSource();
    }

    // Drop target
    handleDragDrop(node);

    // Render children if open
    if (node_open && has_children) {
        for (const auto& child : node->GetChildren()) {
            renderNode(child.get());
        }
        ImGui::TreePop();
    }
}

void SceneHierarchyPanel::handleDragDrop(SceneNode* node)
{
    if (!ImGui::BeginDragDropTarget()) {
        return;
    }

    if (const ImGuiPayload* payload =
            ImGui::AcceptDragDropPayload("SCENE_NODE"))
    {
        auto* dropped_node = *static_cast<SceneNode**>(payload->Data);

        // Validate drop - can't drop onto self or own descendant
        bool valid_drop = (dropped_node != nullptr && dropped_node != node);

        if (valid_drop) {
            // Check if node is a descendant of dropped_node
            SceneNode* check = node;
            while (check != nullptr) {
                if (check == dropped_node) {
                    valid_drop = false;
                    break;
                }
                check = check->GetParent();
            }
        }

        if (valid_drop) {
            Logger::Info("Reparenting '{}' under '{}'",
                         dropped_node->GetName(),
                         node->GetName());

            // Remove from current parent and add to new parent
            SceneNode* old_parent = dropped_node->GetParent();
            if (old_parent != nullptr) {
                auto& siblings = const_cast<std::vector<std::unique_ptr<SceneNode>>&>(
                    old_parent->GetChildren());
                for (auto it = siblings.begin(); it != siblings.end(); ++it) {
                    if (it->get() == dropped_node) {
                        auto node_ptr = std::move(*it);
                        siblings.erase(it);
                        node->AddChild(std::move(node_ptr));
                        break;
                    }
                }
            }
        }
    }
    ImGui::EndDragDropTarget();
}

auto SceneHierarchyPanel::GetSelectedNode() const -> SceneNode*
{
    return m_SelectedNode;
}

void SceneHierarchyPanel::SetSelectedNode(SceneNode* node)
{
    if (m_SelectedNode != node) {
        m_SelectedNode = node;
        if (m_OnSelectionChanged) {
            m_OnSelectionChanged(node);
        }
    }
}

void SceneHierarchyPanel::SetOnSelectionChanged(
    std::function<void(SceneNode*)> callback)
{
    m_OnSelectionChanged = std::move(callback);
}
```

**Step 3: Add source file to CMakeLists.txt**

In `CMakeLists.txt`, add after SettingsPanel.cpp:

```cmake
        source/UI/SceneHierarchyPanel.cpp
```

**Step 4: Verify build compiles**

Run: `task build`

Expected: Build may fail - need to complete backend implementations first.

**Step 5: Commit UI panels together**

```bash
git add include/UI/RHIImGui.hpp source/UI/RHIImGui.cpp \
        include/UI/SettingsPanel.hpp source/UI/SettingsPanel.cpp \
        include/UI/SceneHierarchyPanel.hpp source/UI/SceneHierarchyPanel.cpp \
        include/Renderer/Camera.hpp CMakeLists.txt
git commit -m "$(cat <<'EOF'
feat(ui): add RHIImGui interface and panel classes

- RHIImGui: Abstract interface for backend-specific ImGui
- SettingsPanel: Collapsible sidebar with renderer, camera, debug sections
- SceneHierarchyPanel: Tree view with drag-drop reparenting
- Add FOV, near/far plane getters to Camera

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>
EOF
)"
```

---

## Task 7: Create VulkanImGui Backend

**Files:**
- Create: `include/Renderer/RHI/Vulkan/VulkanImGui.hpp`
- Create: `source/Renderer/RHI/Vulkan/VulkanImGui.cpp`

**Step 1: Create the header file**

Create `include/Renderer/RHI/Vulkan/VulkanImGui.hpp`:

```cpp
#ifndef RENDERER_RHI_VULKAN_VULKANIMGUI_HPP
#define RENDERER_RHI_VULKAN_VULKANIMGUI_HPP

#include <volk.h>

#include "UI/RHIImGui.hpp"

class VulkanDevice;

class VulkanImGui : public RHIImGui
{
public:
    explicit VulkanImGui(VulkanDevice* device);

    void Init(Window* window) override;
    void Shutdown() override;
    void BeginFrame() override;
    void EndFrame() override;

private:
    VulkanDevice* m_Device = nullptr;
    VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
};

#endif
```

**Step 2: Create the implementation file**

Create `source/Renderer/RHI/Vulkan/VulkanImGui.cpp`:

```cpp
#include "Renderer/RHI/Vulkan/VulkanImGui.hpp"

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>

#include "Core/Logger.hpp"
#include "Core/Window.hpp"
#include "Renderer/RHI/Vulkan/VulkanDevice.hpp"
#include "Renderer/RHI/Vulkan/VulkanSwapchain.hpp"
#include "Renderer/RHI/Vulkan/VulkanUtils.hpp"
#include "UI/ImGuiStyle.hpp"

VulkanImGui::VulkanImGui(VulkanDevice* device)
    : m_Device(device)
{
}

void VulkanImGui::Init(Window* window)
{
    Logger::Info("Initializing Vulkan ImGui backend");

    // Create descriptor pool for ImGui
    std::array<VkDescriptorPoolSize, 1> pool_sizes = {{
        {.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
         .descriptorCount = 100},
    }};

    VkDescriptorPoolCreateInfo pool_info {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 100;
    pool_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
    pool_info.pPoolSizes = pool_sizes.data();

    if (auto result = VkUtils::Check(vkCreateDescriptorPool(
            m_Device->GetVkDevice(), &pool_info, nullptr, &m_DescriptorPool));
        !result)
    {
        throw std::runtime_error("Failed to create ImGui descriptor pool");
    }

    // Setup ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& imgui_io = ImGui::GetIO();
    imgui_io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Apply flat theme
    ImGuiStyle::ApplyFlatTheme();

    // Initialize SDL3 backend
    auto* sdl_window = static_cast<SDL_Window*>(window->GetNativeWindow());
    ImGui_ImplSDL3_InitForVulkan(sdl_window);

    // Initialize Vulkan backend
    auto* swapchain = static_cast<VulkanSwapchain*>(m_Device->GetSwapchain());

    ImGui_ImplVulkan_InitInfo init_info {};
    init_info.Instance = m_Device->GetVkInstance();
    init_info.PhysicalDevice = m_Device->GetVkPhysicalDevice();
    init_info.Device = m_Device->GetVkDevice();
    init_info.QueueFamily = m_Device->GetGraphicsQueueFamily();
    init_info.Queue = m_Device->GetGraphicsQueue();
    init_info.DescriptorPool = m_DescriptorPool;
    init_info.MinImageCount = swapchain->GetImageCount();
    init_info.ImageCount = swapchain->GetImageCount();
    init_info.UseDynamicRendering = true;

    // Dynamic rendering format
    init_info.PipelineRenderingCreateInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    init_info.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
    VkFormat color_format = swapchain->GetVkFormat();
    init_info.PipelineRenderingCreateInfo.pColorAttachmentFormats =
        &color_format;

    ImGui_ImplVulkan_Init(&init_info);

    Logger::Info("Vulkan ImGui backend initialized");
}

void VulkanImGui::Shutdown()
{
    Logger::Info("Shutting down Vulkan ImGui backend");

    m_Device->WaitIdle();

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    if (m_DescriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(
            m_Device->GetVkDevice(), m_DescriptorPool, nullptr);
        m_DescriptorPool = VK_NULL_HANDLE;
    }
}

void VulkanImGui::BeginFrame()
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
}

void VulkanImGui::EndFrame()
{
    ImGui::Render();

    auto* cmd_buffer = static_cast<VulkanCommandBuffer*>(
        m_Device->GetCurrentCommandBuffer());

    ImGui_ImplVulkan_RenderDrawData(
        ImGui::GetDrawData(), cmd_buffer->GetHandle());
}
```

**Step 3: Add VulkanDevice accessor methods**

Add to `include/Renderer/RHI/Vulkan/VulkanDevice.hpp` in the public section:

```cpp
  [[nodiscard]] auto GetVkInstance() const -> VkInstance { return m_Instance; }
  [[nodiscard]] auto GetVkPhysicalDevice() const -> VkPhysicalDevice
  {
      return m_PhysicalDevice;
  }
  [[nodiscard]] auto GetVkDevice() const -> VkDevice { return m_Device; }
  [[nodiscard]] auto GetGraphicsQueueFamily() const -> uint32_t
  {
      return m_GraphicsQueueFamily;
  }
  [[nodiscard]] auto GetGraphicsQueue() const -> VkQueue
  {
      return m_GraphicsQueue;
  }
```

**Step 4: Add VulkanSwapchain GetVkFormat method**

Add to `include/Renderer/RHI/Vulkan/VulkanSwapchain.hpp`:

```cpp
  [[nodiscard]] auto GetVkFormat() const -> VkFormat { return m_SwapchainFormat; }
```

**Step 5: Add VulkanCommandBuffer GetHandle method (if not exists)**

Check `include/Renderer/RHI/Vulkan/VulkanCommandBuffer.hpp` and add if missing:

```cpp
  [[nodiscard]] auto GetHandle() const -> VkCommandBuffer { return m_CommandBuffer; }
```

**Step 6: Add source file to CMakeLists.txt**

In `CMakeLists.txt`, add after VulkanSampler.cpp:

```cmake
        source/Renderer/RHI/Vulkan/VulkanImGui.cpp
```

**Step 7: Commit**

```bash
git add include/Renderer/RHI/Vulkan/VulkanImGui.hpp \
        source/Renderer/RHI/Vulkan/VulkanImGui.cpp \
        include/Renderer/RHI/Vulkan/VulkanDevice.hpp \
        include/Renderer/RHI/Vulkan/VulkanSwapchain.hpp \
        include/Renderer/RHI/Vulkan/VulkanCommandBuffer.hpp \
        CMakeLists.txt
git commit -m "$(cat <<'EOF'
feat(ui): add VulkanImGui backend implementation

- Creates dedicated descriptor pool for ImGui
- Uses dynamic rendering with VK_KHR_dynamic_rendering
- Integrates with SDL3 for input handling

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>
EOF
)"
```

---

## Task 8: Create OpenGLImGui Backend

**Files:**
- Create: `include/Renderer/RHI/OpenGL/OpenGLImGui.hpp`
- Create: `source/Renderer/RHI/OpenGL/OpenGLImGui.cpp`

**Step 1: Create the header file**

Create `include/Renderer/RHI/OpenGL/OpenGLImGui.hpp`:

```cpp
#ifndef RENDERER_RHI_OPENGL_OPENGLIMGUI_HPP
#define RENDERER_RHI_OPENGL_OPENGLIMGUI_HPP

#include "UI/RHIImGui.hpp"

class OpenGLDevice;

class OpenGLImGui : public RHIImGui
{
public:
    explicit OpenGLImGui(OpenGLDevice* device);

    void Init(Window* window) override;
    void Shutdown() override;
    void BeginFrame() override;
    void EndFrame() override;

private:
    OpenGLDevice* m_Device = nullptr;
};

#endif
```

**Step 2: Create the implementation file**

Create `source/Renderer/RHI/OpenGL/OpenGLImGui.cpp`:

```cpp
#include "Renderer/RHI/OpenGL/OpenGLImGui.hpp"

#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl3.h>

#include "Core/Logger.hpp"
#include "Core/Window.hpp"
#include "Renderer/RHI/OpenGL/OpenGLDevice.hpp"
#include "UI/ImGuiStyle.hpp"

OpenGLImGui::OpenGLImGui(OpenGLDevice* device)
    : m_Device(device)
{
}

void OpenGLImGui::Init(Window* window)
{
    Logger::Info("Initializing OpenGL ImGui backend");

    // Setup ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& imgui_io = ImGui::GetIO();
    imgui_io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Apply flat theme
    ImGuiStyle::ApplyFlatTheme();

    // Initialize SDL3 backend
    auto* sdl_window = static_cast<SDL_Window*>(window->GetNativeWindow());
    ImGui_ImplSDL3_InitForOpenGL(sdl_window, m_Device->GetGLContext());

    // Initialize OpenGL3 backend with GLSL 460
    ImGui_ImplOpenGL3_Init("#version 460");

    Logger::Info("OpenGL ImGui backend initialized");
}

void OpenGLImGui::Shutdown()
{
    Logger::Info("Shutting down OpenGL ImGui backend");

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
}

void OpenGLImGui::BeginFrame()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
}

void OpenGLImGui::EndFrame()
{
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
```

**Step 3: Add OpenGLDevice GetGLContext method**

Add to `include/Renderer/RHI/OpenGL/OpenGLDevice.hpp`:

```cpp
  [[nodiscard]] auto GetGLContext() const -> SDL_GLContext { return m_GLContext; }
```

**Step 4: Add source file to CMakeLists.txt**

In `CMakeLists.txt`, add after OpenGLSampler.cpp:

```cmake
        source/Renderer/RHI/OpenGL/OpenGLImGui.cpp
```

**Step 5: Commit**

```bash
git add include/Renderer/RHI/OpenGL/OpenGLImGui.hpp \
        source/Renderer/RHI/OpenGL/OpenGLImGui.cpp \
        include/Renderer/RHI/OpenGL/OpenGLDevice.hpp \
        CMakeLists.txt
git commit -m "$(cat <<'EOF'
feat(ui): add OpenGLImGui backend implementation

- Uses OpenGL 4.6 / GLSL 460 for shaders
- Integrates with SDL3 for input handling

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>
EOF
)"
```

---

## Task 9: Implement RHIImGui::Create Factory

**Files:**
- Modify: `source/UI/RHIImGui.cpp`

**Step 1: Add includes and implement Create**

At the top of `source/UI/RHIImGui.cpp`, add includes:

```cpp
#include "Renderer/RHI/OpenGL/OpenGLDevice.hpp"
#include "Renderer/RHI/OpenGL/OpenGLImGui.hpp"
#include "Renderer/RHI/Vulkan/VulkanDevice.hpp"
#include "Renderer/RHI/Vulkan/VulkanImGui.hpp"
```

Add the Create implementation before the constructor:

```cpp
auto RHIImGui::Create(RHIDevice* device) -> std::unique_ptr<RHIImGui>
{
    // Check device type and create appropriate backend
    if (auto* vulkan_device = dynamic_cast<VulkanDevice*>(device)) {
        return std::make_unique<VulkanImGui>(vulkan_device);
    }
    if (auto* opengl_device = dynamic_cast<OpenGLDevice*>(device)) {
        return std::make_unique<OpenGLImGui>(opengl_device);
    }

    Logger::Error("Unknown RHI device type for ImGui");
    return nullptr;
}
```

**Step 2: Verify build compiles**

Run: `task build`

Expected: Build succeeds with no errors.

**Step 3: Commit**

```bash
git add source/UI/RHIImGui.cpp
git commit -m "$(cat <<'EOF'
feat(ui): implement RHIImGui factory method

Creates VulkanImGui or OpenGLImGui based on device type.

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>
EOF
)"
```

---

## Task 10: Integrate ImGui into Application

**Files:**
- Modify: `include/Core/Application.hpp`
- Modify: `source/Core/Application.cpp`

**Step 1: Add RHIImGui member to Application**

In `include/Core/Application.hpp`, add include and member:

After line 9 (after `class RHIDevice;`):
```cpp
class RHIImGui;
```

After line 50 (after `m_RHIDevice`):
```cpp
  std::unique_ptr<RHIImGui> m_ImGui;
```

Add protected accessor after GetWindow (around line 43):
```cpp
  [[nodiscard]] auto GetImGui() -> RHIImGui* { return m_ImGui.get(); }
```

**Step 2: Initialize ImGui in Application::Init**

In `source/Core/Application.cpp`, add include after line 10:
```cpp
#include "UI/RHIImGui.hpp"
```

After swapchain creation (after line 30), add:
```cpp
  m_ImGui = RHIImGui::Create(m_RHIDevice.get());
  m_ImGui->Init(m_Window.get());
```

**Step 3: Shutdown ImGui in Application::Destroy**

In `source/Core/Application.cpp`, after WaitIdle (around line 47), add:
```cpp
  if (m_ImGui) {
    m_ImGui->Shutdown();
    m_ImGui.reset();
  }
```

**Step 4: Add ImGui to render loop**

In `source/Core/Application.cpp`, in the Run() method:

After `OnUpdate(delta_time);` (line 105), add:
```cpp
    // Process ImGui input
    // Note: ImGui input is handled in backend BeginFrame via SDL events
```

After `m_RHIDevice->BeginFrame();` (line 108), add:
```cpp
    m_ImGui->BeginFrame();
```

Before `m_RHIDevice->EndFrame();` (line 113), add:
```cpp
    m_ImGui->EndFrame();
```

**Step 5: Handle keyboard shortcuts for panel toggles**

In the Run() loop, after `OnUpdate(delta_time);`, add:
```cpp
    // Handle UI shortcuts
    if (Input::IsKeyPressed(KeyCode::F1)) {
      m_ImGui->ToggleSettings();
    }
    if (Input::IsKeyPressed(KeyCode::F2)) {
      m_ImGui->ToggleSceneHierarchy();
    }
```

**Step 6: Verify build compiles**

Run: `task build`

Expected: Build succeeds with no errors.

**Step 7: Commit**

```bash
git add include/Core/Application.hpp source/Core/Application.cpp
git commit -m "$(cat <<'EOF'
feat(ui): integrate ImGui into Application lifecycle

- Initialize ImGui after swapchain creation
- BeginFrame/EndFrame in render loop
- F1/F2 keyboard shortcuts for panel toggles
- Proper shutdown order

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>
EOF
)"
```

---

## Task 11: Update scene_demo.cpp Example

**Files:**
- Modify: `example/scene_demo.cpp`

**Step 1: Add ImGui panel rendering**

In `example/scene_demo.cpp`, add include after line 18:
```cpp
#include "UI/RHIImGui.hpp"
```

In `OnInit()`, after camera setup (around line 84), add:
```cpp
    // Setup ImGui
    GetImGui()->SetCamera(&m_Camera);
```

In `OnRender()`, before the scene rendering, add:
```cpp
    // Render UI panels
    GetImGui()->RenderPanels(m_Scene.get());
```

**Step 2: Update OnUpdate for performance stats**

In `OnUpdate()`, add after `m_Scene->UpdateTransforms();`:
```cpp
    // Update performance stats
    PerformanceStats stats {};
    stats.FrameTime = delta_time;
    stats.Fps = 1.0F / delta_time;
    stats.DrawCalls = 0;  // TODO: Track draw calls
    stats.MemoryUsageMb = 0;  // TODO: Track memory
    // GetImGui()->GetSettingsPanel()->UpdateStats(stats);
```

Note: Stats update requires exposing GetSettingsPanel() - skip for now.

**Step 3: Remove duplicate F1/F2 handling**

The Application base class now handles F1/F2, so no changes needed in scene_demo.cpp.

**Step 4: Verify build and run**

Run: `task run`

Expected: Application runs with ImGui panels accessible via F1/F2 or edge buttons.

**Step 5: Commit**

```bash
git add example/scene_demo.cpp
git commit -m "$(cat <<'EOF'
feat(example): integrate ImGui panels into scene_demo

- Connect camera to settings panel
- Render UI panels in OnRender

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>
EOF
)"
```

---

## Task 12: Add SDL Event Forwarding to ImGui

**Files:**
- Modify: `source/Core/Application.cpp`

**Step 1: Forward SDL events to ImGui**

ImGui needs to see SDL events for mouse/keyboard input. In `source/Core/Application.cpp`, modify `OnEvent`:

```cpp
void Application::OnEvent(void* event)
{
  // Forward to ImGui first
  ImGui_ImplSDL3_ProcessEvent(static_cast<SDL_Event*>(event));

  // Then process for game input
  Input::ProcessEvent(event);
}
```

Add include at top:
```cpp
#include <imgui_impl_sdl3.h>
```

**Step 2: Verify build and test**

Run: `task run`

Expected: ImGui responds to mouse clicks and keyboard input.

**Step 3: Commit**

```bash
git add source/Core/Application.cpp
git commit -m "$(cat <<'EOF'
fix(ui): forward SDL events to ImGui for input handling

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>
EOF
)"
```

---

## Task 13: Final Verification and Cleanup

**Step 1: Run full build**

Run: `task rebuild`

Expected: Clean build succeeds.

**Step 2: Run clang-format**

Run: `task format`

Expected: All files formatted correctly.

**Step 3: Run clang-tidy**

Run: `task tidy`

Expected: No warnings (or only pre-existing ones).

**Step 4: Test both backends**

Edit `config.toml` and set `api = "vulkan"`, run `task run`.
Edit `config.toml` and set `api = "opengl"`, run `task run`.

Expected: Both backends work with ImGui panels.

**Step 5: Final commit if any formatting changes**

```bash
git add -A
git commit -m "$(cat <<'EOF'
chore: apply formatting fixes

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>
EOF
)"
```

---

## Summary

This plan adds:

1. **ImGui dependency** via vcpkg with SDL3, Vulkan, and OpenGL3 bindings
2. **ImGuiStyle** - Flat minimalist theme inspired by Blender 3.0+
3. **RHIImGui** - Abstract interface with factory pattern
4. **SettingsPanel** - Collapsible sidebar with renderer/camera/debug sections
5. **SceneHierarchyPanel** - Tree view with drag-drop reparenting
6. **VulkanImGui** - Vulkan backend with dynamic rendering
7. **OpenGLImGui** - OpenGL 4.6 backend
8. **Application integration** - Lifecycle management and keyboard shortcuts

Total: 13 tasks with incremental commits.
