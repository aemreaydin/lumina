#ifndef RENDERER_SCENE_TRANSFORM_HPP
#define RENDERER_SCENE_TRANSFORM_HPP

#include <linalg/mat3.hpp>
#include <linalg/mat4.hpp>
#include <linalg/quaternion.hpp>
#include <linalg/vec.hpp>

class Transform
{
public:
  Transform() = default;

  // Local transform setters
  void SetPosition(const linalg::Vec3& position);
  void SetRotation(const linalg::Quat& rotation);
  void SetRotationEuler(const linalg::Vec3& euler_degrees);
  void SetScale(const linalg::Vec3& scale);
  void SetScale(float uniform_scale);

  // Local transform getters
  [[nodiscard]] auto GetPosition() const -> const linalg::Vec3&;
  [[nodiscard]] auto GetRotation() const -> const linalg::Quat&;
  [[nodiscard]] auto GetRotationEuler() const -> linalg::Vec3;
  [[nodiscard]] auto GetScale() const -> const linalg::Vec3&;

  // Transform operations
  void Translate(const linalg::Vec3& delta);
  void Rotate(const linalg::Quat& delta);
  void RotateEuler(const linalg::Vec3& euler_degrees);
  void ScaleBy(const linalg::Vec3& factor);
  void ScaleBy(float uniform_factor);

  // Direction vectors (in local space)
  [[nodiscard]] auto GetForward() const -> linalg::Vec3;
  [[nodiscard]] auto GetRight() const -> linalg::Vec3;
  [[nodiscard]] auto GetUp() const -> linalg::Vec3;

  // Matrix access - call UpdateMatrices() first to ensure they're current
  [[nodiscard]] auto GetLocalMatrix() const -> const linalg::Mat4&;
  [[nodiscard]] auto GetWorldMatrix() const -> const linalg::Mat4&;
  [[nodiscard]] auto GetNormalMatrix() const -> linalg::Mat3;

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
  void LookAt(const linalg::Vec3& target,
              const linalg::Vec3& v_up = {0.0F, 1.0F, 0.0F});

  // Interpolation
  static auto Lerp(const Transform& from, const Transform& to, float t)
      -> Transform;

private:
  linalg::Vec3 m_Position {0.0F, 0.0F, 0.0F};
  linalg::Quat m_Rotation {0.0F, 0.0F, 0.0F, 1.0F};  // Identity quaternion
  linalg::Vec3 m_Scale {1.0F, 1.0F, 1.0F};

  linalg::Mat4 m_LocalMatrix {linalg::Mat4::identity()};
  linalg::Mat4 m_WorldMatrix {linalg::Mat4::identity()};
  bool m_LocalDirty {true};
  bool m_WorldDirty {true};

  Transform* m_Parent {nullptr};
};

#endif
