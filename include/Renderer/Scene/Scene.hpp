#ifndef RENDERER_SCENE_SCENE_HPP
#define RENDERER_SCENE_SCENE_HPP

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "Renderer/Model/BoundingVolume.hpp"
#include "Renderer/Scene/SceneNode.hpp"

class Model;
class Camera;

class Scene
{
public:
  explicit Scene(std::string name = "Scene");
  ~Scene();

  Scene(const Scene&) = delete;
  Scene(Scene&&) noexcept;
  auto operator=(const Scene&) -> Scene& = delete;
  auto operator=(Scene&&) noexcept -> Scene&;

  // Root node access
  [[nodiscard]] auto GetRoot() -> SceneNode&;
  [[nodiscard]] auto GetRoot() const -> const SceneNode&;

  // Convenience methods for creating nodes
  auto CreateNode(const std::string& name = "Node", SceneNode* parent = nullptr)
      -> SceneNode*;

  // Find nodes by name
  [[nodiscard]] auto FindNode(const std::string& name) const -> SceneNode*;

  // Pick the closest visible node intersected by a ray
  [[nodiscard]] auto PickNode(const Ray& ray) const -> SceneNode*;

  // Scene properties
  void SetName(const std::string& name);
  [[nodiscard]] auto GetName() const -> const std::string&;

  // Update all transforms in the scene
  void UpdateTransforms();

  // Scene bounds (union of all visible node bounds)
  [[nodiscard]] auto GetBounds() const -> AABB;

  // Traverse all nodes (depth-first)
  void ForEachNode(const std::function<void(SceneNode&)>& callback);
  void ForEachNode(const std::function<void(const SceneNode&)>& callback) const;

  // Get all visible nodes with models (for rendering)
  [[nodiscard]] auto GetRenderableNodes() const -> std::vector<SceneNode*>;

  // Statistics
  [[nodiscard]] auto GetNodeCount() const -> size_t;
  [[nodiscard]] auto GetVisibleNodeCount() const -> size_t;

  // Active camera
  void SetActiveCamera(Camera* camera);
  [[nodiscard]] auto GetActiveCamera() const -> Camera*;

  // Generate a unique node name (appends _1, _2, ... if name already exists)
  [[nodiscard]] auto MakeUniqueName(const std::string& name) const
      -> std::string;

private:
  auto count_nodes(const SceneNode& node) const -> size_t;

  std::string m_Name;
  std::unique_ptr<SceneNode> m_Root;
  Camera* m_ActiveCamera {nullptr};
};

#endif
