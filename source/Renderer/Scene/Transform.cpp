#include "Renderer/Scene/Transform.hpp"

#include <linalg/mat3.hpp>
#include <linalg/quaternion.hpp>
#include <linalg/transform.hpp>
#include <linalg/utility.hpp>

void Transform::SetPosition(const linalg::Vec3& position)
{
  m_Position = position;
  MarkDirty();
}

void Transform::SetRotation(const linalg::Quat& rotation)
{
  m_Rotation = linalg::normalized(rotation);
  MarkDirty();
}

void Transform::SetRotationEuler(const linalg::Vec3& euler_degrees)
{
  const linalg::Vec3 radians = linalg::radians(euler_degrees);
  m_Rotation = linalg::quat_from_euler(radians);
  MarkDirty();
}

void Transform::SetScale(const linalg::Vec3& scale)
{
  m_Scale = scale;
  MarkDirty();
}

void Transform::SetScale(float uniform_scale)
{
  m_Scale = linalg::Vec3{uniform_scale, uniform_scale, uniform_scale};
  MarkDirty();
}

auto Transform::GetPosition() const -> const linalg::Vec3&
{
  return m_Position;
}

auto Transform::GetRotation() const -> const linalg::Quat&
{
  return m_Rotation;
}

auto Transform::GetRotationEuler() const -> linalg::Vec3
{
  return linalg::degrees(linalg::euler_angles(m_Rotation));
}

auto Transform::GetScale() const -> const linalg::Vec3&
{
  return m_Scale;
}

void Transform::Translate(const linalg::Vec3& delta)
{
  m_Position += delta;
  MarkDirty();
}

void Transform::Rotate(const linalg::Quat& delta)
{
  m_Rotation = linalg::normalized(delta * m_Rotation);
  MarkDirty();
}

void Transform::RotateEuler(const linalg::Vec3& euler_degrees)
{
  const linalg::Vec3 radians = linalg::radians(euler_degrees);
  Rotate(linalg::quat_from_euler(radians));
}

void Transform::ScaleBy(const linalg::Vec3& factor)
{
  m_Scale *= factor;
  MarkDirty();
}

void Transform::ScaleBy(float uniform_factor)
{
  m_Scale *= uniform_factor;
  MarkDirty();
}

auto Transform::GetForward() const -> linalg::Vec3
{
  return linalg::normalized(linalg::transform(linalg::Vec3{0.0F, 0.0F, -1.0F}, m_Rotation));
}

auto Transform::GetRight() const -> linalg::Vec3
{
  return linalg::normalized(linalg::transform(linalg::Vec3{1.0F, 0.0F, 0.0F}, m_Rotation));
}

auto Transform::GetUp() const -> linalg::Vec3
{
  return linalg::normalized(linalg::transform(linalg::Vec3{0.0F, 1.0F, 0.0F}, m_Rotation));
}

auto Transform::GetLocalMatrix() const -> const linalg::Mat4&
{
  return m_LocalMatrix;
}

auto Transform::GetWorldMatrix() const -> const linalg::Mat4&
{
  return m_WorldMatrix;
}

auto Transform::GetNormalMatrix() const -> linalg::Mat3
{
  const linalg::Mat3 upper3x3{
      m_WorldMatrix(0, 0), m_WorldMatrix(0, 1), m_WorldMatrix(0, 2),
      m_WorldMatrix(1, 0), m_WorldMatrix(1, 1), m_WorldMatrix(1, 2),
      m_WorldMatrix(2, 0), m_WorldMatrix(2, 1), m_WorldMatrix(2, 2)};
  return linalg::transpose(linalg::inverse(upper3x3));
}

void Transform::UpdateLocalMatrix()
{
  if (!m_LocalDirty) {
    return;
  }

  m_LocalMatrix = linalg::Mat4::identity();
  m_LocalMatrix = m_LocalMatrix * static_cast<linalg::Mat4>(linalg::make_translation(m_Position));
  m_LocalMatrix = m_LocalMatrix * m_Rotation.to_mat4();
  m_LocalMatrix = m_LocalMatrix * static_cast<linalg::Mat4>(linalg::make_scale(m_Scale));
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

void Transform::LookAt(const linalg::Vec3& target, const linalg::Vec3& v_up)
{
  const linalg::Vec3 direction = linalg::normalized(target - m_Position);

  // Calculate rotation to face the target
  const linalg::Vec3 right = linalg::normalized(linalg::cross(v_up, direction));
  const linalg::Vec3 actual_up = linalg::cross(direction, right);

  linalg::Mat3 rotation_matrix{right, actual_up, -direction};

  linalg::Quat q;
  q.set_rotation_from_matrix(rotation_matrix);
  m_Rotation = linalg::normalized(q);
  MarkDirty();
}

auto Transform::Lerp(const Transform& from, const Transform& to, float t)
    -> Transform
{
  Transform result;
  result.m_Position = linalg::mix(from.m_Position, to.m_Position, t);
  result.m_Rotation = linalg::slerp(from.m_Rotation, to.m_Rotation, t);
  result.m_Scale = linalg::mix(from.m_Scale, to.m_Scale, t);
  result.MarkDirty();
  return result;
}
