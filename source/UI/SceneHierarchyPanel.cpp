#include "UI/SceneHierarchyPanel.hpp"

#include <imgui.h>

#include "Core/Logger.hpp"
#include "Renderer/Scene/Scene.hpp"
#include "Renderer/Scene/SceneNode.hpp"

void SceneHierarchyPanel::Render(Scene& scene, float animation_progress)
{
  if (animation_progress <= 0.0F) {
    return;
  }

  ImGui::PushStyleVar(ImGuiStyleVar_Alpha, animation_progress);

  constexpr ImGuiWindowFlags WINDOW_FLAGS = ImGuiWindowFlags_NoTitleBar
      | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove
      | ImGuiWindowFlags_NoCollapse;

  if (ImGui::Begin("Scene Hierarchy", nullptr, WINDOW_FLAGS)) {
    ImGui::Text("SCENE HIERARCHY");
    ImGui::Separator();

    ImGui::Text("%s", scene.GetName().c_str());
    ImGui::Separator();

    // Render the root node's children (root itself is usually not shown)
    SceneNode& root = scene.GetRoot();
    for (const auto& child : root.GetChildren()) {
      renderNode(child.get());
    }

    // Handle drop on empty space (unparent to root)
    if (ImGui::BeginDragDropTarget()) {
      if (const ImGuiPayload* payload =
              ImGui::AcceptDragDropPayload("SCENE_NODE"))
      {
        auto* dropped_node = *static_cast<SceneNode**>(payload->Data);
        if (dropped_node != nullptr && dropped_node->GetParent() != &root) {
          Logger::Info("Reparenting '{}' to root", dropped_node->GetName());

          // Remove from current parent and add to root
          SceneNode* old_parent = dropped_node->GetParent();
          if (old_parent != nullptr) {
            // Find and move the unique_ptr
            auto& siblings =
                const_cast<std::vector<std::unique_ptr<SceneNode>>&>(
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
      | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_FramePadding;

  if (!has_children) {
    flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
  }

  // Draw full-row selection highlight (before widgets so it renders behind)
  if (is_selected) {
    ImVec2 row_min(ImGui::GetWindowPos().x, ImGui::GetCursorScreenPos().y);
    ImVec2 row_max(row_min.x + ImGui::GetWindowWidth(),
                   row_min.y + ImGui::GetFrameHeight());
    ImGui::GetWindowDrawList()->AddRectFilled(
        row_min, row_max, ImGui::GetColorU32(ImGuiCol_Header));
  }

  const bool node_open = ImGui::TreeNodeEx(
      static_cast<void*>(node), flags, "%s", node->GetName().c_str());

  // Selection on tree node click (right after TreeNodeEx, before checkbox)
  if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
    SetSelectedNode(node);
  }

  // Drag source (on tree node)
  if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
    m_DraggedNode = node;
    ImGui::SetDragDropPayload("SCENE_NODE", &node, sizeof(SceneNode*));
    ImGui::Text("Moving: %s", node->GetName().c_str());
    ImGui::EndDragDropSource();
  }

  // Drop target
  handleDragDrop(node, nullptr);

  // Visibility checkbox aligned to the right edge
  constexpr float CHECKBOX_WIDTH = 20.0F;
  ImGui::SameLine(ImGui::GetWindowWidth() - CHECKBOX_WIDTH
                  - ImGui::GetStyle().WindowPadding.x);
  bool is_visible = node->IsVisible();
  if (ImGui::Checkbox(("##vis_" + node->GetName()).c_str(), &is_visible)) {
    node->SetVisible(is_visible);
  }

  // Render children if open
  if (node_open && has_children) {
    for (const auto& child : node->GetChildren()) {
      renderNode(child.get());
    }
    ImGui::TreePop();
  }
}

void SceneHierarchyPanel::handleDragDrop(SceneNode* node, SceneNode* /*root*/)
{
  if (!ImGui::BeginDragDropTarget()) {
    return;
  }

  if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SCENE_NODE"))
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
