#include <algorithm>
#include <cmath>
#include <limits>

#include "Renderer/CameraController.hpp"

#include "Core/Input.hpp"
#include "Renderer/Camera.hpp"

// FPSCameraController implementation

FPSCameraController::FPSCameraController(Camera* camera)
    : CameraController(camera)
{
}

void FPSCameraController::Update(float delta_time)
{
  if (m_Camera == nullptr) {
    return;
  }

  // Mouse look (only when right mouse button is held or mouse is captured)
  if (Input::IsMouseCaptured() || Input::IsMouseButtonDown(MouseButton::Right))
  {
    const glm::vec2 mouse_delta = Input::GetMouseDelta();
    m_Camera->Rotate(-mouse_delta.y * m_LookSensitivity,
                     mouse_delta.x * m_LookSensitivity);
  }

  // Toggle mouse capture with right click
  if (Input::IsMouseButtonPressed(MouseButton::Right)) {
    Input::SetMouseCaptured(/*captured=*/true);
  }
  if (Input::IsMouseButtonReleased(MouseButton::Right)) {
    Input::SetMouseCaptured(/*captured=*/false);
  }

  // Escape releases mouse capture
  if (Input::IsKeyPressed(KeyCode::Escape)) {
    Input::SetMouseCaptured(/*captured=*/false);
  }

  // WASD movement
  glm::vec3 movement {0.0F};

  if (Input::IsKeyDown(KeyCode::W)) {
    movement.y += 1.0F;  // Forward
  }
  if (Input::IsKeyDown(KeyCode::S)) {
    movement.y -= 1.0F;  // Backward
  }
  if (Input::IsKeyDown(KeyCode::A)) {
    movement.x -= 1.0F;  // Left
  }
  if (Input::IsKeyDown(KeyCode::D)) {
    movement.x += 1.0F;  // Right
  }
  if (Input::IsKeyDown(KeyCode::Space)) {
    movement.z += 1.0F;  // Up
  }
  if (Input::IsKeyDown(KeyCode::LCtrl)) {
    movement.z -= 1.0F;  // Down
  }

  // Normalize diagonal movement
  if (glm::length(movement) > 0.0F) {
    movement = glm::normalize(movement);
  }

  // Apply speed modifier with shift
  float speed = m_MoveSpeed;
  if (Input::IsKeyDown(KeyCode::LShift)) {
    speed *= 2.0F;
  }

  m_Camera->MoveRelative(movement * speed * delta_time);
}

// OrbitCameraController implementation

OrbitCameraController::OrbitCameraController(Camera* camera)
    : CameraController(camera)
{
  update_camera_position();
}

void OrbitCameraController::Update([[maybe_unused]] float delta_time)
{
  if (m_Camera == nullptr) {
    return;
  }

  // Orbit with left mouse button drag
  if (Input::IsMouseButtonDown(MouseButton::Left)) {
    const glm::vec2 mouse_delta = Input::GetMouseDelta();
    m_Azimuth += mouse_delta.x * m_OrbitSpeed;
    m_Elevation -= mouse_delta.y * m_OrbitSpeed;

    // Clamp elevation to avoid flipping
    m_Elevation = std::clamp(m_Elevation, -89.0F, 89.0F);
  }

  // Zoom with scroll wheel
  const glm::vec2 scroll = Input::GetScrollDelta();
  if (std::fabs(scroll.y) > std::numeric_limits<float>::epsilon()) {
    // Create window with render API info
    m_Distance -= scroll.y * m_ZoomSpeed;
    m_Distance = std::clamp(m_Distance, m_MinDistance, m_MaxDistance);
  }

  update_camera_position();
}

void OrbitCameraController::SetTarget(const glm::vec3& target)
{
  m_Target = target;
  update_camera_position();
}

void OrbitCameraController::SetDistance(float distance)
{
  m_Distance = std::clamp(distance, m_MinDistance, m_MaxDistance);
  update_camera_position();
}

void OrbitCameraController::SetDistanceLimits(float min_distance,
                                              float max_distance)
{
  m_MinDistance = min_distance;
  m_MaxDistance = max_distance;
  m_Distance = std::clamp(m_Distance, m_MinDistance, m_MaxDistance);
}

void OrbitCameraController::update_camera_position()
{
  if (m_Camera == nullptr) {
    return;
  }

  // Convert spherical coordinates to Cartesian
  // Z-up coordinate system
  const float azimuth_rad = glm::radians(m_Azimuth);
  const float elevation_rad = glm::radians(m_Elevation);

  const float horizontal_dist = m_Distance * std::cos(elevation_rad);
  const float x = m_Target.x + (horizontal_dist * std::cos(azimuth_rad));
  const float y = m_Target.y + (horizontal_dist * std::sin(azimuth_rad));
  const float z = m_Target.z + (m_Distance * std::sin(elevation_rad));

  m_Camera->SetPosition(glm::vec3(x, y, z));
  m_Camera->SetTarget(m_Target);
}
