#ifndef RENDERER_SCENE_TRANSFORM_HPP
#define RENDERER_SCENE_TRANSFORM_HPP

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

class Transform
{
public:
  Transform() = default;

  // Local transform setters
  void SetPosition(const glm::vec3& position);
  void SetRotation(const glm::quat& rotation);
  void SetRotationEuler(const glm::vec3& euler_degrees);
  void SetScale(const glm::vec3& scale);
  void SetScale(float uniform_scale);

  // Local transform getters
  [[nodiscard]] auto GetPosition() const -> const glm::vec3&;
  [[nodiscard]] auto GetRotation() const -> const glm::quat&;
  [[nodiscard]] auto GetRotationEuler() const -> glm::vec3;
  [[nodiscard]] auto GetScale() const -> const glm::vec3&;

  // Transform operations
  void Translate(const glm::vec3& delta);
  void Rotate(const glm::quat& delta);
  void RotateEuler(const glm::vec3& euler_degrees);
  void ScaleBy(const glm::vec3& factor);
  void ScaleBy(float uniform_factor);

  // Direction vectors (in local space)
  [[nodiscard]] auto GetForward() const -> glm::vec3;
  [[nodiscard]] auto GetRight() const -> glm::vec3;
  [[nodiscard]] auto GetUp() const -> glm::vec3;

  // Matrix access - call UpdateMatrices() first to ensure they're current
  [[nodiscard]] auto GetLocalMatrix() const -> const glm::mat4&;
  [[nodiscard]] auto GetWorldMatrix() const -> const glm::mat4&;
  [[nodiscard]] auto GetNormalMatrix() const -> glm::mat3;

  // Compute matrices from TRS components (call after modifying transform)
  void UpdateLocalMatrix();
  void UpdateWorldMatrix();
  void UpdateMatrices();

  // Parent transform for hierarchy
  void SetParent(Transform* parent);
  [[nodiscard]] auto GetParent() const -> Transform*;

  // Dirty tracking
  void MarkDirty();
  [[nodiscard]] auto IsLocalDirty() const -> bool;
  [[nodiscard]] auto IsWorldDirty() const -> bool;

  // Look at a target point
  void LookAt(const glm::vec3& target,
              const glm::vec3& v_up = {0.0F, 1.0F, 0.0F});

  // Interpolation
  static auto Lerp(const Transform& from, const Transform& to, float t)
      -> Transform;

private:
  glm::vec3 m_Position {0.0F, 0.0F, 0.0F};
  glm::quat m_Rotation {1.0F, 0.0F, 0.0F, 0.0F};  // Identity quaternion
  glm::vec3 m_Scale {1.0F, 1.0F, 1.0F};

  glm::mat4 m_LocalMatrix {1.0F};
  glm::mat4 m_WorldMatrix {1.0F};
  bool m_LocalDirty {true};
  bool m_WorldDirty {true};

  Transform* m_Parent {nullptr};
};

#endif
