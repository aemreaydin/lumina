#include <algorithm>
#include <cmath>

#include "Renderer/Camera.hpp"

#include <glm/gtc/matrix_transform.hpp>

Camera::Camera()
{
  update_direction_vectors();
  recalculate_view_matrix();
  recalculate_projection_matrix();
}

void Camera::SetPosition(const glm::vec3& position)
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

void Camera::SetTarget(const glm::vec3& target)
{
  const glm::vec3 direction = glm::normalize(target - m_Position);

  m_Pitch = glm::degrees(std::asin(direction.z));

  m_Yaw = glm::degrees(std::atan2(direction.y, direction.x));

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

auto Camera::GetPosition() const -> const glm::vec3&
{
  return m_Position;
}

auto Camera::GetForward() const -> const glm::vec3&
{
  return m_Forward;
}

auto Camera::GetRight() const -> const glm::vec3&
{
  return m_Right;
}

auto Camera::GetUp() const -> const glm::vec3&
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

auto Camera::GetViewMatrix() const -> const glm::mat4&
{
  return m_ViewMatrix;
}

auto Camera::GetProjectionMatrix() const -> const glm::mat4&
{
  return m_ProjectionMatrix;
}

auto Camera::GetViewProjectionMatrix() const -> glm::mat4
{
  return m_ProjectionMatrix * m_ViewMatrix;
}

void Camera::Move(const glm::vec3& offset)
{
  m_Position += offset;
  recalculate_view_matrix();
}

void Camera::MoveRelative(const glm::vec3& offset)
{
  m_Position += m_Right * offset.x;
  m_Position += m_Forward * offset.y;
  m_Position += m_Up * offset.z;
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
  m_ViewMatrix = glm::lookAt(m_Position, m_Position + m_Forward, WORLD_UP);
}

void Camera::recalculate_projection_matrix()
{
  if (m_ProjectionType == ProjectionType::Perspective) {
    m_ProjectionMatrix = glm::perspective(
        glm::radians(m_FOV), m_AspectRatio, m_NearPlane, m_FarPlane);
  } else {
    m_ProjectionMatrix = glm::ortho(m_OrthoLeft,
                                    m_OrthoRight,
                                    m_OrthoBottom,
                                    m_OrthoTop,
                                    m_NearPlane,
                                    m_FarPlane);
  }
}

void Camera::update_direction_vectors()
{
  const float pitch_rad = glm::radians(m_Pitch);
  const float yaw_rad = glm::radians(m_Yaw);

  m_Forward.x = std::cos(pitch_rad) * std::cos(yaw_rad);
  m_Forward.y = std::cos(pitch_rad) * std::sin(yaw_rad);
  m_Forward.z = std::sin(pitch_rad);
  m_Forward = glm::normalize(m_Forward);

  m_Right = glm::normalize(glm::cross(m_Forward, WORLD_UP));

  m_Up = glm::normalize(glm::cross(m_Right, m_Forward));
}
