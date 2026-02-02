#ifndef UI_SCENEHIERARCHYPANEL_HPP
#define UI_SCENEHIERARCHYPANEL_HPP

#include <functional>
#include <unordered_set>

class Scene;
class SceneNode;

class SceneHierarchyPanel
{
public:
  void Render(Scene& scene, float animation_progress);

  [[nodiscard]] auto GetSelectedNode() const -> SceneNode*;
  void SetSelectedNode(SceneNode* node);

  void SetOnSelectionChanged(std::function<void(SceneNode*)> callback);

private:
  void renderNode(SceneNode* node);
  void handleDragDrop(SceneNode* node, SceneNode* root);

  SceneNode* m_SelectedNode = nullptr;
  SceneNode* m_DraggedNode = nullptr;
  std::function<void(SceneNode*)> m_OnSelectionChanged;
  std::unordered_set<SceneNode*> m_NodesToExpand;
};

#endif
