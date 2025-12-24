#include "Renderer/Scene/Transform.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/matrix_decompose.hpp>

void Transform::SetPosition(const glm::vec3& position)
{
  m_Position = position;
  MarkDirty();
}

void Transform::SetRotation(const glm::quat& rotation)
{
  m_Rotation = glm::normalize(rotation);
  MarkDirty();
}

void Transform::SetRotationEuler(const glm::vec3& euler_degrees)
{
  const glm::vec3 radians = glm::radians(euler_degrees);
  m_Rotation = glm::quat(radians);
  MarkDirty();
}

void Transform::SetScale(const glm::vec3& scale)
{
  m_Scale = scale;
  MarkDirty();
}

void Transform::SetScale(float uniform_scale)
{
  m_Scale = glm::vec3(uniform_scale);
  MarkDirty();
}

auto Transform::GetPosition() const -> const glm::vec3&
{
  return m_Position;
}

auto Transform::GetRotation() const -> const glm::quat&
{
  return m_Rotation;
}

auto Transform::GetRotationEuler() const -> glm::vec3
{
  return glm::degrees(glm::eulerAngles(m_Rotation));
}

auto Transform::GetScale() const -> const glm::vec3&
{
  return m_Scale;
}

void Transform::Translate(const glm::vec3& delta)
{
  m_Position += delta;
  MarkDirty();
}

void Transform::Rotate(const glm::quat& delta)
{
  m_Rotation = glm::normalize(delta * m_Rotation);
  MarkDirty();
}

void Transform::RotateEuler(const glm::vec3& euler_degrees)
{
  const glm::vec3 radians = glm::radians(euler_degrees);
  Rotate(glm::quat(radians));
}

void Transform::ScaleBy(const glm::vec3& factor)
{
  m_Scale *= factor;
  MarkDirty();
}

void Transform::ScaleBy(float uniform_factor)
{
  m_Scale *= uniform_factor;
  MarkDirty();
}

auto Transform::GetForward() const -> glm::vec3
{
  return glm::normalize(m_Rotation * glm::vec3(0.0F, 0.0F, -1.0F));
}

auto Transform::GetRight() const -> glm::vec3
{
  return glm::normalize(m_Rotation * glm::vec3(1.0F, 0.0F, 0.0F));
}

auto Transform::GetUp() const -> glm::vec3
{
  return glm::normalize(m_Rotation * glm::vec3(0.0F, 1.0F, 0.0F));
}

auto Transform::GetLocalMatrix() const -> const glm::mat4&
{
  return m_LocalMatrix;
}

auto Transform::GetWorldMatrix() const -> const glm::mat4&
{
  return m_WorldMatrix;
}

auto Transform::GetNormalMatrix() const -> glm::mat3
{
  return glm::transpose(glm::inverse(glm::mat3(m_WorldMatrix)));
}

void Transform::UpdateLocalMatrix()
{
  if (!m_LocalDirty) {
    return;
  }

  m_LocalMatrix = glm::mat4(1.0F);
  m_LocalMatrix = glm::translate(m_LocalMatrix, m_Position);
  m_LocalMatrix *= glm::mat4_cast(m_Rotation);
  m_LocalMatrix = glm::scale(m_LocalMatrix, m_Scale);
  m_LocalDirty = false;
}

void Transform::UpdateWorldMatrix()
{
  if (!m_WorldDirty) {
    return;
  }

  // Ensure local matrix is up to date
  UpdateLocalMatrix();

  if (m_Parent != nullptr) {
    m_WorldMatrix = m_Parent->GetWorldMatrix() * m_LocalMatrix;
  } else {
    m_WorldMatrix = m_LocalMatrix;
  }

  m_WorldDirty = false;
}

void Transform::UpdateMatrices()
{
  UpdateLocalMatrix();
  UpdateWorldMatrix();
}

void Transform::SetParent(Transform* parent)
{
  m_Parent = parent;
  m_WorldDirty = true;
}

auto Transform::GetParent() const -> Transform*
{
  return m_Parent;
}

void Transform::MarkDirty()
{
  m_LocalDirty = true;
  m_WorldDirty = true;
}

auto Transform::IsLocalDirty() const -> bool
{
  return m_LocalDirty;
}

auto Transform::IsWorldDirty() const -> bool
{
  return m_WorldDirty;
}

void Transform::LookAt(const glm::vec3& target, const glm::vec3& v_up)
{
  const glm::vec3 direction = glm::normalize(target - m_Position);

  // Calculate rotation to face the target
  const glm::vec3 right = glm::normalize(glm::cross(v_up, direction));
  const glm::vec3 actual_up = glm::cross(direction, right);

  glm::mat3 rotation_matrix;
  rotation_matrix[0] = right;
  rotation_matrix[1] = actual_up;
  rotation_matrix[2] = -direction;

  m_Rotation = glm::normalize(glm::quat_cast(rotation_matrix));
  MarkDirty();
}

auto Transform::Lerp(const Transform& from, const Transform& to, float t)
    -> Transform
{
  Transform result;
  result.m_Position = glm::mix(from.m_Position, to.m_Position, t);
  result.m_Rotation = glm::slerp(from.m_Rotation, to.m_Rotation, t);
  result.m_Scale = glm::mix(from.m_Scale, to.m_Scale, t);
  result.MarkDirty();
  return result;
}
