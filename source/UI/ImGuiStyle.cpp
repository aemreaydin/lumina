#include "UI/ImGuiStyle.hpp"

#include <imgui.h>

namespace UIStyle
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
  colors[ImGuiCol_WindowBg] = ImVec4(0.145F, 0.145F, 0.145F, 1.0F);  // #252525
  colors[ImGuiCol_ChildBg] = ImVec4(0.114F, 0.114F, 0.114F, 1.0F);  // #1D1D1D
  colors[ImGuiCol_PopupBg] = ImVec4(0.145F, 0.145F, 0.145F, 1.0F);

  // Borders
  colors[ImGuiCol_Border] = ImVec4(0.25F, 0.25F, 0.25F, 1.0F);
  colors[ImGuiCol_BorderShadow] = ImVec4(0.0F, 0.0F, 0.0F, 0.0F);

  // Frame backgrounds (inputs, sliders)
  colors[ImGuiCol_FrameBg] = ImVec4(0.2F, 0.2F, 0.2F, 1.0F);  // #333333
  colors[ImGuiCol_FrameBgHovered] =
      ImVec4(0.267F, 0.267F, 0.267F, 1.0F);  // #444444
  colors[ImGuiCol_FrameBgActive] =
      ImVec4(0.314F, 0.314F, 0.314F, 1.0F);  // #505050

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
  colors[ImGuiCol_HeaderHovered] =
      ImVec4(0.29F, 0.435F, 0.647F, 0.8F);  // Accent blue
  colors[ImGuiCol_HeaderActive] =
      ImVec4(0.29F, 0.435F, 0.647F, 1.0F);  // #4A6FA5

  // Tabs
  colors[ImGuiCol_Tab] = ImVec4(0.145F, 0.145F, 0.145F, 1.0F);
  colors[ImGuiCol_TabHovered] = ImVec4(0.29F, 0.435F, 0.647F, 0.8F);
  colors[ImGuiCol_TabActive] = ImVec4(0.2F, 0.2F, 0.2F, 1.0F);
  colors[ImGuiCol_TabUnfocused] = ImVec4(0.114F, 0.114F, 0.114F, 1.0F);
  colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.145F, 0.145F, 0.145F, 1.0F);

  // Text
  colors[ImGuiCol_Text] = ImVec4(0.878F, 0.878F, 0.878F, 1.0F);  // #E0E0E0
  colors[ImGuiCol_TextDisabled] =
      ImVec4(0.439F, 0.439F, 0.439F, 1.0F);  // #707070

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

}  // namespace UIStyle
