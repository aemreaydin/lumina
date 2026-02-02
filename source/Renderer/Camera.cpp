#include <algorithm>
#include <cmath>

#include "Renderer/Camera.hpp"

#include <linalg/mat4.hpp>
#include <linalg/projection.hpp>
#include <linalg/utility.hpp>

Camera::Camera()
{
  update_direction_vectors();
  recalculate_view_matrix();
  recalculate_projection_matrix();
}

void Camera::SetPosition(const linalg::Vec3& position)
{
  m_Position = position;
  recalculate_view_matrix();
}

void Camera::SetRotation(float pitch, float yaw)
{
  m_Pitch = pitch;
  m_Yaw = yaw;
  update_direction_vectors();
  recalculate_view_matrix();
}

void Camera::SetTarget(const linalg::Vec3& target)
{
  const linalg::Vec3 direction = linalg::normalized(target - m_Position);

  m_Pitch = linalg::degrees(std::asin(direction.z()));

  m_Yaw = linalg::degrees(std::atan2(direction.y(), direction.x()));

  update_direction_vectors();
  recalculate_view_matrix();
}

void Camera::SetPerspective(float fov_y,
                            float aspect_ratio,
                            float near_plane,
                            float far_plane)
{
  m_ProjectionType = ProjectionType::Perspective;
  m_FOV = fov_y;
  m_AspectRatio = aspect_ratio;
  m_NearPlane = near_plane;
  m_FarPlane = far_plane;
  recalculate_projection_matrix();
}

void Camera::SetOrthographic(float left,
                             float right,
                             float bottom,
                             float top,
                             float near_plane,
                             float far_plane)
{
  m_ProjectionType = ProjectionType::Orthographic;
  m_OrthoLeft = left;
  m_OrthoRight = right;
  m_OrthoBottom = bottom;
  m_OrthoTop = top;
  m_NearPlane = near_plane;
  m_FarPlane = far_plane;
  recalculate_projection_matrix();
}

void Camera::SetAspectRatio(float aspect_ratio)
{
  m_AspectRatio = aspect_ratio;
  recalculate_projection_matrix();
}

auto Camera::GetPosition() const -> const linalg::Vec3&
{
  return m_Position;
}

auto Camera::GetForward() const -> const linalg::Vec3&
{
  return m_Forward;
}

auto Camera::GetRight() const -> const linalg::Vec3&
{
  return m_Right;
}

auto Camera::GetUp() const -> const linalg::Vec3&
{
  return m_Up;
}

auto Camera::GetPitch() const -> float
{
  return m_Pitch;
}

auto Camera::GetYaw() const -> float
{
  return m_Yaw;
}

auto Camera::GetViewMatrix() const -> const linalg::Mat4&
{
  return m_ViewMatrix;
}

auto Camera::GetProjectionMatrix() const -> const linalg::Mat4&
{
  return m_ProjectionMatrix;
}

auto Camera::GetViewProjectionMatrix() const -> linalg::Mat4
{
  return m_ProjectionMatrix * m_ViewMatrix;
}

auto Camera::ScreenPointToRay(float screen_x,
                              float screen_y,
                              float viewport_w,
                              float viewport_h) const -> Ray
{
  // Convert screen coordinates to NDC (-1 to 1, Y flipped)
  const float ndc_x = (screen_x / viewport_w) * 2.0F - 1.0F;
  const float ndc_y = 1.0F - (screen_y / viewport_h) * 2.0F;

  const linalg::Mat4 inv_vp = linalg::inverse(GetViewProjectionMatrix());

  // Unproject near and far points (depth 0-1 due to
  // GLM_FORCE_DEPTH_ZERO_TO_ONE)
  const linalg::Vec4 near_clip{ndc_x, ndc_y, 0.0F, 1.0F};
  const linalg::Vec4 far_clip{ndc_x, ndc_y, 1.0F, 1.0F};

  linalg::Vec4 near_world = inv_vp * near_clip;
  linalg::Vec4 far_world = inv_vp * far_clip;

  near_world /= near_world.w();
  far_world /= far_world.w();

  Ray ray;
  ray.Origin = near_world.to_sub_vec<3>();
  ray.Direction = linalg::normalized((far_world - near_world).to_sub_vec<3>());
  return ray;
}

void Camera::Move(const linalg::Vec3& offset)
{
  m_Position += offset;
  recalculate_view_matrix();
}

void Camera::MoveRelative(const linalg::Vec3& offset)
{
  m_Position += m_Right * offset.x();
  m_Position += m_Forward * offset.y();
  m_Position += m_Up * offset.z();
  recalculate_view_matrix();
}

void Camera::Rotate(float delta_pitch, float delta_yaw)
{
  m_Pitch += delta_pitch;
  m_Yaw += delta_yaw;

  m_Pitch = std::clamp(m_Pitch, -89.0F, 89.0F);

  update_direction_vectors();
  recalculate_view_matrix();
}

void Camera::recalculate_view_matrix()
{
  m_ViewMatrix = linalg::look_at(m_Position, m_Position + m_Forward, WORLD_UP);
}

void Camera::recalculate_projection_matrix()
{
  if (m_ProjectionType == ProjectionType::Perspective) {
    m_ProjectionMatrix = linalg::perspective(
        linalg::radians(m_FOV), m_AspectRatio, m_NearPlane, m_FarPlane);
  } else {
    m_ProjectionMatrix = linalg::ortho(m_OrthoLeft,
                                    m_OrthoRight,
                                    m_OrthoBottom,
                                    m_OrthoTop,
                                    m_NearPlane,
                                    m_FarPlane);
  }
}

void Camera::update_direction_vectors()
{
  const float pitch_rad = linalg::radians(m_Pitch);
  const float yaw_rad = linalg::radians(m_Yaw);

  m_Forward.x() = std::cos(pitch_rad) * std::cos(yaw_rad);
  m_Forward.y() = std::cos(pitch_rad) * std::sin(yaw_rad);
  m_Forward.z() = std::sin(pitch_rad);
  m_Forward = linalg::normalized(m_Forward);

  m_Right = linalg::normalized(linalg::cross(m_Forward, WORLD_UP));

  m_Up = linalg::normalized(linalg::cross(m_Right, m_Forward));
}
