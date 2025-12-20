#ifndef RENDERER_SCENE_SCENENODE_HPP
#define RENDERER_SCENE_SCENENODE_HPP

#include <memory>
#include <string>
#include <vector>

#include "Renderer/Model/BoundingVolume.hpp"
#include "Renderer/Scene/Transform.hpp"

class Model;

class SceneNode
{
public:
  explicit SceneNode(std::string name = "Node");
  ~SceneNode();

  SceneNode(const SceneNode&) = delete;
  SceneNode(SceneNode&&) noexcept;
  auto operator=(const SceneNode&) -> SceneNode& = delete;
  auto operator=(SceneNode&&) noexcept -> SceneNode&;

  // Hierarchy management
  auto AddChild(std::unique_ptr<SceneNode> child) -> SceneNode*;
  auto CreateChild(const std::string& name = "Node") -> SceneNode*;
  void RemoveChild(SceneNode* child);
  void RemoveFromParent();
  void ClearChildren();

  [[nodiscard]] auto GetParent() const -> SceneNode*;
  [[nodiscard]] auto GetChildren() const
      -> const std::vector<std::unique_ptr<SceneNode>>&;
  [[nodiscard]] auto GetChildCount() const -> size_t;
  [[nodiscard]] auto FindChild(const std::string& name) const -> SceneNode*;
  [[nodiscard]] auto FindChildRecursive(const std::string& name) const
      -> SceneNode*;

  // Transform access
  [[nodiscard]] auto GetTransform() -> Transform&;
  [[nodiscard]] auto GetTransform() const -> const Transform&;

  // Convenience transform methods
  void SetPosition(const glm::vec3& position);
  void SetRotation(const glm::quat& rotation);
  void SetRotationEuler(const glm::vec3& euler_degrees);
  void SetScale(const glm::vec3& scale);
  void SetScale(float uniform_scale);

  [[nodiscard]] auto GetPosition() const -> const glm::vec3&;
  [[nodiscard]] auto GetWorldPosition() const -> glm::vec3;

  // Model attachment
  void SetModel(std::shared_ptr<Model> model);
  [[nodiscard]] auto GetModel() const -> std::shared_ptr<Model>;
  [[nodiscard]] auto HasModel() const -> bool;

  // Node properties
  void SetName(const std::string& name);
  [[nodiscard]] auto GetName() const -> const std::string&;

  void SetVisible(bool visible);
  [[nodiscard]] auto IsVisible() const -> bool;
  [[nodiscard]] auto IsVisibleInHierarchy() const -> bool;

  void SetEnabled(bool enabled);
  [[nodiscard]] auto IsEnabled() const -> bool;

  // Bounding volume
  [[nodiscard]] auto GetLocalBounds() const -> AABB;
  [[nodiscard]] auto GetWorldBounds() const -> AABB;

  // Update hierarchy transforms (call once per frame before rendering)
  void UpdateTransforms();

private:
  void set_parent(SceneNode* parent);

  std::string m_Name;
  Transform m_Transform;
  SceneNode* m_Parent {nullptr};
  std::vector<std::unique_ptr<SceneNode>> m_Children;

  std::shared_ptr<Model> m_Model;

  bool m_Visible {true};
  bool m_Enabled {true};
};

#endif
