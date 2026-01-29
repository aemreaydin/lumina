# ImGui UI System Design

## Overview

Add a UI system using ImGui with RHI integration, supporting both Vulkan and OpenGL backends. The system provides collapsible sidebar panels for settings/debug info and scene hierarchy management.

## Requirements

- RHI-integrated ImGui (VulkanImGui + OpenGLImGui implementations)
- Flat minimalist styling (Blender 3.0+ / Unity dark theme)
- Collapsible sidebars with slide animations
- Toggle via keyboard shortcuts (F1, F2) and edge buttons
- Settings panel with renderer, camera, and debug sections
- Scene Hierarchy panel with drag-drop reparenting

## Architecture

### File Structure

```
include/UI/
├── RHIImGui.hpp              # Abstract interface
├── ImGuiStyle.hpp            # Theme function declaration
├── SettingsPanel.hpp         # Settings panel class
└── SceneHierarchyPanel.hpp   # Scene tree panel class

source/UI/
├── RHIImGui.cpp              # Shared logic, factory
├── ImGuiStyle.cpp            # Flat theme implementation
├── SettingsPanel.cpp         # Renderer/camera/debug sections
└── SceneHierarchyPanel.cpp   # Tree view, drag-drop logic

source/Renderer/RHI/Vulkan/
└── VulkanImGui.cpp           # Vulkan backend

source/Renderer/RHI/OpenGL/
└── OpenGLImGui.cpp           # OpenGL backend
```

### Abstract Interface

```cpp
#ifndef UI_RHIIMGUI_HPP
#define UI_RHIIMGUI_HPP

#include <memory>

class Window;
class Scene;

class RHIImGui
{
public:
    static auto Create() -> std::unique_ptr<RHIImGui>;

    virtual void Init(Window* window) = 0;
    virtual void Shutdown() = 0;
    virtual void BeginFrame() = 0;
    virtual void EndFrame() = 0;

    void RenderPanels(Scene* scene);
    void ToggleSettings();
    void ToggleSceneHierarchy();

    RHIImGui(const RHIImGui&) = delete;
    RHIImGui(RHIImGui&&) = delete;
    auto operator=(const RHIImGui&) -> RHIImGui& = delete;
    auto operator=(RHIImGui&&) -> RHIImGui& = delete;
    virtual ~RHIImGui() = default;

protected:
    RHIImGui() = default;

    std::unique_ptr<SettingsPanel> m_SettingsPanel;
    std::unique_ptr<SceneHierarchyPanel> m_SceneHierarchyPanel;
    bool m_ShowSettings = false;
    bool m_ShowSceneHierarchy = false;
};

#endif
```

## Components

### Settings Panel

```cpp
#ifndef UI_SETTINGSPANEL_HPP
#define UI_SETTINGSPANEL_HPP

class RHIDevice;
class Camera;

struct PerformanceStats
{
    float FrameTime;
    float Fps;
    uint32_t DrawCalls;
    uint64_t MemoryUsageMb;
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
    PerformanceStats m_Stats{};

    // Debug toggles
    bool m_Wireframe = false;
    bool m_ShowBoundingBoxes = false;
    bool m_ShowNormals = false;
};

#endif
```

**Sections:**

1. **Renderer** - API name (read-only), VSync toggle, resolution display, validation status
2. **Camera** - FOV slider, near/far plane inputs, speed/sensitivity sliders
3. **Debug** - FPS/frame time, draw calls, memory usage, wireframe toggle, bounding boxes toggle, normals toggle

### Scene Hierarchy Panel

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

**Behavior:**

- Tree view with expand/collapse arrows for nodes with children
- Click to select (highlights the node)
- Drag a node onto another to make it a child
- Drag a node to root level to unparent it
- Selected node is visually distinct (highlight color)
- Hover feedback during drag operations (valid/invalid drop target)

## Styling

### Theme Configuration

```cpp
#ifndef UI_IMGUISTYLE_HPP
#define UI_IMGUISTYLE_HPP

namespace ImGuiStyle
{

void ApplyFlatTheme();

} // namespace ImGuiStyle

#endif
```

### Color Palette

| Element | Color |
|---------|-------|
| Background | `#1D1D1D` |
| Panel background | `#252525` |
| Widget background | `#333333` |
| Widget hover | `#444444` |
| Widget active | `#505050` |
| Accent (selection) | `#4A6FA5` |
| Text | `#E0E0E0` |
| Disabled text | `#707070` |

### Widget Styling

- Border radius: 2px (minimal rounding)
- No gradients - flat solid colors only
- 1px borders on inputs and buttons
- Minimal frame padding - compact but readable

### Sidebar Behavior

- Edge toggle buttons: Small, icon-based
- Slide animation: ~150ms ease-out transition
- Settings panel: Right edge
- Scene Hierarchy panel: Left edge

## Backend Implementations

### VulkanImGui

```cpp
class VulkanImGui : public RHIImGui
{
public:
    void Init(Window* window) override;
    void Shutdown() override;
    void BeginFrame() override;
    void EndFrame() override;

private:
    VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
};
```

**Vulkan-specific:**

- Creates dedicated `VkDescriptorPool` for ImGui
- Uses `ImGui_ImplVulkan_Init` with existing device, queue, render pass
- `EndFrame()` calls `ImGui_ImplVulkan_RenderDrawData` into current command buffer

### OpenGLImGui

```cpp
class OpenGLImGui : public RHIImGui
{
public:
    void Init(Window* window) override;
    void Shutdown() override;
    void BeginFrame() override;
    void EndFrame() override;
};
```

**OpenGL-specific:**

- Uses `ImGui_ImplOpenGL3_Init` with GLSL version string
- No descriptor pools needed
- `EndFrame()` calls `ImGui_ImplOpenGL3_RenderDrawData`

### Factory

```cpp
auto RHIImGui::Create() -> std::unique_ptr<RHIImGui>
{
    if (config.api == "vulkan") {
        return std::make_unique<VulkanImGui>();
    }
    return std::make_unique<OpenGLImGui>();
}
```

## Integration

### Initialization

```cpp
// In Application::Init()
m_ImGui = RHIImGui::Create();
m_ImGui->Init(m_Window.get());

m_ImGui->GetSettingsPanel()->SetDevice(m_Device.get());
m_ImGui->GetSettingsPanel()->SetCamera(m_Camera.get());
```

### Render Loop

```cpp
m_Device->BeginFrame();

m_SceneRenderer->Render(scene);

m_ImGui->BeginFrame();
m_ImGui->RenderPanels(m_Scene.get());
m_ImGui->EndFrame();

m_Device->EndFrame();
m_Device->Present();
```

### Input Handling

```cpp
ImGui_ImplSDL3_ProcessEvent(&event);

if (event.type == SDL_EVENT_KEY_DOWN) {
    if (event.key.key == SDLK_F1) m_ImGui->ToggleSettings();
    if (event.key.key == SDLK_F2) m_ImGui->ToggleSceneHierarchy();
}
```

### Stats Update

```cpp
PerformanceStats stats{};
stats.FrameTime = m_DeltaTime;
stats.Fps = 1.0f / m_DeltaTime;
stats.DrawCalls = m_Renderer->GetDrawCallCount();
stats.MemoryUsageMb = m_Device->GetMemoryUsage();
m_ImGui->GetSettingsPanel()->UpdateStats(stats);
```

## Dependencies

Add to `vcpkg.json`:

```json
"imgui": {
  "features": ["sdl3-binding", "vulkan-binding", "opengl3-binding"]
}
```

## Keyboard Shortcuts

| Key | Action |
|-----|--------|
| F1 | Toggle Settings panel |
| F2 | Toggle Scene Hierarchy panel |

## Future Expansion

The design supports future additions:

- **Inspector/Properties panel** - `SetOnSelectionChanged` callback ready for integration
- **Additional panels** - Same collapsible sidebar pattern can be reused
- **Theme switching** - Style system can support multiple presets
