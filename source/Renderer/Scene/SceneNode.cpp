#include <algorithm>

#include "Renderer/Scene/SceneNode.hpp"

#include "Renderer/Model/Model.hpp"

SceneNode::SceneNode(std::string name)
    : m_Name(std::move(name))
{
}

SceneNode::~SceneNode() = default;

SceneNode::SceneNode(SceneNode&&) noexcept = default;
auto SceneNode::operator=(SceneNode&&) noexcept -> SceneNode& = default;

auto SceneNode::AddChild(std::unique_ptr<SceneNode> child) -> SceneNode*
{
  if (!child) {
    return nullptr;
  }

  child->set_parent(this);
  m_Children.push_back(std::move(child));
  return m_Children.back().get();
}

auto SceneNode::CreateChild(const std::string& name) -> SceneNode*
{
  return AddChild(std::make_unique<SceneNode>(name));
}

void SceneNode::RemoveChild(SceneNode* child)
{
  if (child == nullptr) {
    return;
  }

  auto iter = std::ranges::find_if(m_Children,
                                   [child](const auto& ptr) -> auto
                                   { return ptr.get() == child; });

  if (iter != m_Children.end()) {
    (*iter)->set_parent(nullptr);
    m_Children.erase(iter);
  }
}

void SceneNode::RemoveFromParent()
{
  if (m_Parent != nullptr) {
    m_Parent->RemoveChild(this);
  }
}

void SceneNode::ClearChildren()
{
  for (auto& child : m_Children) {
    child->set_parent(nullptr);
  }
  m_Children.clear();
}

auto SceneNode::GetParent() const -> SceneNode*
{
  return m_Parent;
}

auto SceneNode::GetChildren() const
    -> const std::vector<std::unique_ptr<SceneNode>>&
{
  return m_Children;
}

auto SceneNode::GetChildCount() const -> size_t
{
  return m_Children.size();
}

auto SceneNode::FindChild(const std::string& name) const -> SceneNode*
{
  for (const auto& child : m_Children) {
    if (child->GetName() == name) {
      return child.get();
    }
  }
  return nullptr;
}

auto SceneNode::FindChildRecursive(const std::string& name) const -> SceneNode*
{
  // Check immediate children first
  auto* found = FindChild(name);
  if (found != nullptr) {
    return found;
  }

  // Recursively search children
  for (const auto& child : m_Children) {
    found = child->FindChildRecursive(name);
    if (found != nullptr) {
      return found;
    }
  }

  return nullptr;
}

auto SceneNode::GetTransform() -> Transform&
{
  return m_Transform;
}

auto SceneNode::GetTransform() const -> const Transform&
{
  return m_Transform;
}

void SceneNode::SetPosition(const glm::vec3& position)
{
  m_Transform.SetPosition(position);
}

void SceneNode::SetRotation(const glm::quat& rotation)
{
  m_Transform.SetRotation(rotation);
}

void SceneNode::SetRotationEuler(const glm::vec3& euler_degrees)
{
  m_Transform.SetRotationEuler(euler_degrees);
}

void SceneNode::SetScale(const glm::vec3& scale)
{
  m_Transform.SetScale(scale);
}

void SceneNode::SetScale(float uniform_scale)
{
  m_Transform.SetScale(uniform_scale);
}

auto SceneNode::GetPosition() const -> const glm::vec3&
{
  return m_Transform.GetPosition();
}

auto SceneNode::GetWorldPosition() const -> glm::vec3
{
  return m_Transform.GetWorldMatrix()[3];
}

void SceneNode::SetModel(std::shared_ptr<Model> model)
{
  m_Model = std::move(model);
}

auto SceneNode::GetModel() const -> std::shared_ptr<Model>
{
  return m_Model;
}

auto SceneNode::HasModel() const -> bool
{
  return m_Model != nullptr;
}

void SceneNode::SetName(const std::string& name)
{
  m_Name = name;
}

auto SceneNode::GetName() const -> const std::string&
{
  return m_Name;
}

void SceneNode::SetVisible(bool visible)
{
  m_Visible = visible;
}

auto SceneNode::IsVisible() const -> bool
{
  return m_Visible;
}

auto SceneNode::IsVisibleInHierarchy() const -> bool
{
  if (!m_Visible) {
    return false;
  }
  if (m_Parent != nullptr) {
    return m_Parent->IsVisibleInHierarchy();
  }
  return true;
}

void SceneNode::SetEnabled(bool enabled)
{
  m_Enabled = enabled;
}

auto SceneNode::IsEnabled() const -> bool
{
  return m_Enabled;
}

auto SceneNode::GetLocalBounds() const -> AABB
{
  if (m_Model) {
    return m_Model->GetBounds();
  }
  return AABB {};
}

auto SceneNode::GetWorldBounds() const -> AABB
{
  AABB local_bounds = GetLocalBounds();
  return local_bounds.Transform(m_Transform.GetWorldMatrix());
}

void SceneNode::UpdateTransforms()
{
  // Update this node's transform
  m_Transform.UpdateMatrices();

  // Recursively update children
  for (auto& child : m_Children) {
    child->UpdateTransforms();
  }
}

void SceneNode::set_parent(SceneNode* parent)
{
  m_Parent = parent;
  if (parent != nullptr) {
    m_Transform.SetParent(&parent->m_Transform);
  } else {
    m_Transform.SetParent(nullptr);
  }
}
