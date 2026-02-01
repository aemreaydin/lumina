#include "Renderer/Scene/Scene.hpp"

Scene::Scene(std::string name)
    : m_Name(std::move(name))
    , m_Root(std::make_unique<SceneNode>("Root"))
{
}

Scene::~Scene() = default;

Scene::Scene(Scene&&) noexcept = default;
auto Scene::operator=(Scene&&) noexcept -> Scene& = default;

auto Scene::GetRoot() -> SceneNode&
{
  return *m_Root;
}

auto Scene::GetRoot() const -> const SceneNode&
{
  return *m_Root;
}

auto Scene::CreateNode(const std::string& name, SceneNode* parent) -> SceneNode*
{
  SceneNode* target_parent = (parent != nullptr) ? parent : m_Root.get();
  return target_parent->CreateChild(MakeUniqueName(name));
}

auto Scene::FindNode(const std::string& name) const -> SceneNode*
{
  if (m_Root->GetName() == name) {
    return m_Root.get();
  }
  return m_Root->FindChildRecursive(name);
}

void Scene::SetName(const std::string& name)
{
  m_Name = name;
}

auto Scene::GetName() const -> const std::string&
{
  return m_Name;
}

void Scene::UpdateTransforms()
{
  m_Root->UpdateTransforms();
}

auto Scene::GetBounds() const -> AABB
{
  AABB bounds;
  ForEachNode(
      [&bounds](const SceneNode& node) -> void
      {
        if (node.IsVisibleInHierarchy() && node.HasModel()) {
          bounds.Expand(node.GetWorldBounds());
        }
      });
  return bounds;
}

void Scene::ForEachNode(const std::function<void(SceneNode&)>& callback)
{
  std::function<void(SceneNode&)> traverse = [&](SceneNode& node) -> void
  {
    callback(node);
    for (const auto& child : node.GetChildren()) {
      traverse(*child);
    }
  };
  traverse(*m_Root);
}

void Scene::ForEachNode(
    const std::function<void(const SceneNode&)>& callback) const
{
  std::function<void(const SceneNode&)> traverse =
      [&](const SceneNode& node) -> void
  {
    callback(node);
    for (const auto& child : node.GetChildren()) {
      traverse(*child);
    }
  };
  traverse(*m_Root);
}

auto Scene::GetRenderableNodes() const -> std::vector<SceneNode*>
{
  std::vector<SceneNode*> nodes;
  std::function<void(SceneNode&)> collect = [&](SceneNode& node) -> void
  {
    if (node.IsVisibleInHierarchy() && node.HasModel()) {
      nodes.push_back(&node);
    }
    for (const auto& child : node.GetChildren()) {
      collect(*child);
    }
  };
  collect(*m_Root);
  return nodes;
}

auto Scene::GetNodeCount() const -> size_t
{
  return count_nodes(*m_Root);
}

auto Scene::GetVisibleNodeCount() const -> size_t
{
  size_t count = 0;
  ForEachNode(
      [&count](const SceneNode& node) -> void
      {
        if (node.IsVisibleInHierarchy()) {
          ++count;
        }
      });
  return count;
}

void Scene::SetActiveCamera(Camera* camera)
{
  m_ActiveCamera = camera;
}

auto Scene::GetActiveCamera() const -> Camera*
{
  return m_ActiveCamera;
}

auto Scene::PickNode(const Ray& ray) const -> SceneNode*
{
  const auto nodes = GetRenderableNodes();
  SceneNode* closest = nullptr;
  float closest_t = std::numeric_limits<float>::max();

  for (auto* node : nodes) {
    const AABB bounds = node->GetWorldBounds();
    float t_hit = 0.0F;
    if (bounds.Intersects(ray, t_hit) && t_hit < closest_t) {
      closest_t = t_hit;
      closest = node;
    }
  }

  return closest;
}

auto Scene::MakeUniqueName(const std::string& name) const -> std::string
{
  if (FindNode(name) == nullptr) {
    return name;
  }

  int suffix = 1;
  std::string candidate;
  do {
    candidate = name + "_" + std::to_string(suffix);
    ++suffix;
  } while (FindNode(candidate) != nullptr);

  return candidate;
}

auto Scene::count_nodes(const SceneNode& node) const -> size_t
{
  size_t count = 1;
  for (const auto& child : node.GetChildren()) {
    count += count_nodes(*child);
  }
  return count;
}
