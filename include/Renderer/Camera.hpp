#ifndef RENDERER_CAMERA_HPP
#define RENDERER_CAMERA_HPP

#include <cstdint>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

enum class ProjectionType : uint8_t
{
  Perspective,
  Orthographic
};

class Camera
{
public:
  Camera();

  // Position setters
  void SetPosition(const glm::vec3& position);
  void SetRotation(float pitch, float yaw);
  void SetTarget(const glm::vec3& target);

  // Projection setters
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

  // Getters
  [[nodiscard]] auto GetPosition() const -> const glm::vec3&;
  [[nodiscard]] auto GetForward() const -> const glm::vec3&;
  [[nodiscard]] auto GetRight() const -> const glm::vec3&;
  [[nodiscard]] auto GetUp() const -> const glm::vec3&;
  [[nodiscard]] auto GetPitch() const -> float;
  [[nodiscard]] auto GetYaw() const -> float;

  [[nodiscard]] auto GetViewMatrix() const -> const glm::mat4&;
  [[nodiscard]] auto GetProjectionMatrix() const -> const glm::mat4&;
  [[nodiscard]] auto GetViewProjectionMatrix() const -> glm::mat4;

  // Movement helpers (for controllers)
  void Move(const glm::vec3& offset);
  void MoveRelative(const glm::vec3& offset);
  void Rotate(float delta_pitch, float delta_yaw);

private:
  void recalculate_view_matrix();
  void recalculate_projection_matrix();
  void update_direction_vectors();

  // Transform
  glm::vec3 m_Position {0.0F, 0.0F, 5.0F};
  float m_Pitch {0.0F};
  float m_Yaw {0.0F};

  // Direction vectors
  glm::vec3 m_Forward {0.0F, 1.0F, 0.0F};
  glm::vec3 m_Right {1.0F, 0.0F, 0.0F};
  glm::vec3 m_Up {0.0F, 0.0F, 1.0F};

  // World up vector (Z-up coordinate system)
  static constexpr glm::vec3 WORLD_UP {0.0F, 0.0F, 1.0F};

  // Projection parameters
  ProjectionType m_ProjectionType {ProjectionType::Perspective};
  float m_FOV {45.0F};
  float m_AspectRatio {16.0F / 9.0F};
  float m_NearPlane {0.01F};
  float m_FarPlane {1000.0F};

  // Orthographic bounds
  float m_OrthoLeft {-10.0F};
  float m_OrthoRight {10.0F};
  float m_OrthoBottom {-10.0F};
  float m_OrthoTop {10.0F};

  // Cached matrices
  glm::mat4 m_ViewMatrix {1.0F};
  glm::mat4 m_ProjectionMatrix {1.0F};
};

#endif
