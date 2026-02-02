#ifndef RENDERER_CAMERA_CONTROLLER_HPP
#define RENDERER_CAMERA_CONTROLLER_HPP

#include <linalg/vec.hpp>

class Camera;

class CameraController
{
public:
  explicit CameraController(Camera* camera)
      : m_Camera(camera)
  {
  }

  CameraController(const CameraController&) = default;
  CameraController(CameraController&&) = default;
  auto operator=(const CameraController&) -> CameraController& = default;
  auto operator=(CameraController&&) -> CameraController& = default;
  virtual ~CameraController() = default;

  virtual void Update(float delta_time) = 0;

  void SetCamera(Camera* camera) { m_Camera = camera; }

  [[nodiscard]] auto GetCamera() const -> Camera* { return m_Camera; }

protected:
  Camera* m_Camera {nullptr};
};

// FPS-style camera: WASD movement, mouse look
class FPSCameraController : public CameraController
{
public:
  explicit FPSCameraController(Camera* camera);

  void Update(float delta_time) override;

  void SetMoveSpeed(float speed) { m_MoveSpeed = speed; }

  void SetLookSensitivity(float sensitivity)
  {
    m_LookSensitivity = sensitivity;
  }

  [[nodiscard]] auto GetMoveSpeed() const -> float { return m_MoveSpeed; }

  [[nodiscard]] auto GetLookSensitivity() const -> float
  {
    return m_LookSensitivity;
  }

private:
  float m_MoveSpeed {5.0F};
  float m_LookSensitivity {0.1F};
};

// Orbit camera: rotate around target, scroll to zoom
class OrbitCameraController : public CameraController
{
public:
  explicit OrbitCameraController(Camera* camera);

  void Update(float delta_time) override;

  void SetTarget(const linalg::Vec3& target);
  void SetDistance(float distance);

  void SetOrbitSpeed(float speed) { m_OrbitSpeed = speed; }

  void SetZoomSpeed(float speed) { m_ZoomSpeed = speed; }

  void SetDistanceLimits(float min_distance, float max_distance);

  [[nodiscard]] auto GetTarget() const -> const linalg::Vec3&
  {
    return m_Target;
  }

  [[nodiscard]] auto GetDistance() const -> float { return m_Distance; }

  [[nodiscard]] auto GetAzimuth() const -> float { return m_Azimuth; }

  [[nodiscard]] auto GetElevation() const -> float { return m_Elevation; }

private:
  void update_camera_position();

  linalg::Vec3 m_Target {0.0F, 0.0F, 0.0F};
  float m_Distance {10.0F};
  float m_MinDistance {1.0F};
  float m_MaxDistance {100.0F};
  float m_Azimuth {0.0F};
  float m_Elevation {30.0F};
  float m_OrbitSpeed {0.3F};
  float m_ZoomSpeed {1.0F};
};

#endif
