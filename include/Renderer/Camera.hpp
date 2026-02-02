#ifndef RENDERER_CAMERA_HPP
#define RENDERER_CAMERA_HPP

#include <cstdint>

#include <linalg/vec.hpp>
#include <linalg/mat4.hpp>

#include "Renderer/Model/BoundingVolume.hpp"

enum class ProjectionType : uint8_t
{
  Perspective,
  Orthographic
};

class Camera
{
public:
  Camera();

  void SetPosition(const linalg::Vec3& position);
  void SetRotation(float pitch, float yaw);
  void SetTarget(const linalg::Vec3& target);

  void SetPerspective(float fov_y,
                      float aspect_ratio,
                      float near_plane,
                      float far_plane);
  void SetOrthographic(float left,
                       float right,
                       float bottom,
                       float top,
                       float near_plane,
                       float far_plane);
  void SetAspectRatio(float aspect_ratio);

  [[nodiscard]] auto GetPosition() const -> const linalg::Vec3&;
  [[nodiscard]] auto GetForward() const -> const linalg::Vec3&;
  [[nodiscard]] auto GetRight() const -> const linalg::Vec3&;
  [[nodiscard]] auto GetUp() const -> const linalg::Vec3&;
  [[nodiscard]] auto GetPitch() const -> float;
  [[nodiscard]] auto GetYaw() const -> float;

  [[nodiscard]] auto GetFOV() const -> float { return m_FOV; }

  [[nodiscard]] auto GetNearPlane() const -> float { return m_NearPlane; }

  [[nodiscard]] auto GetFarPlane() const -> float { return m_FarPlane; }

  [[nodiscard]] auto GetViewMatrix() const -> const linalg::Mat4&;
  [[nodiscard]] auto GetProjectionMatrix() const -> const linalg::Mat4&;
  [[nodiscard]] auto GetViewProjectionMatrix() const -> linalg::Mat4;

  [[nodiscard]] auto ScreenPointToRay(float screen_x,
                                      float screen_y,
                                      float viewport_w,
                                      float viewport_h) const -> Ray;

  void Move(const linalg::Vec3& offset);
  void MoveRelative(const linalg::Vec3& offset);
  void Rotate(float delta_pitch, float delta_yaw);

private:
  void recalculate_view_matrix();
  void recalculate_projection_matrix();
  void update_direction_vectors();

  linalg::Vec3 m_Position {0.0F, 0.0F, 5.0F};
  float m_Pitch {0.0F};
  float m_Yaw {0.0F};

  linalg::Vec3 m_Forward {0.0F, 1.0F, 0.0F};
  linalg::Vec3 m_Right {1.0F, 0.0F, 0.0F};
  linalg::Vec3 m_Up {0.0F, 0.0F, 1.0F};

  static constexpr linalg::Vec3 WORLD_UP {0.0F, 0.0F, 1.0F};

  ProjectionType m_ProjectionType {ProjectionType::Perspective};
  float m_FOV {45.0F};
  float m_AspectRatio {16.0F / 9.0F};
  float m_NearPlane {0.01F};
  float m_FarPlane {1000.0F};

  float m_OrthoLeft {-10.0F};
  float m_OrthoRight {10.0F};
  float m_OrthoBottom {-10.0F};
  float m_OrthoTop {10.0F};

  linalg::Mat4 m_ViewMatrix {linalg::Mat4::identity()};
  linalg::Mat4 m_ProjectionMatrix {linalg::Mat4::identity()};
};

#endif
