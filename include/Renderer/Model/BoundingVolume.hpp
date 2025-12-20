#ifndef RENDERER_MODEL_BOUNDINGVOLUME_HPP
#define RENDERER_MODEL_BOUNDINGVOLUME_HPP

#include <limits>
#include <span>

#include <glm/fwd.hpp>
#include <glm/glm.hpp>

// Axis-Aligned Bounding Box
struct AABB
{
  glm::vec3 Min {std::numeric_limits<float>::max()};
  glm::vec3 Max {std::numeric_limits<float>::lowest()};

  [[nodiscard]] auto GetCenter() const -> glm::vec3
  {
    return (Min + Max) * 0.5F;
  }

  [[nodiscard]] auto GetExtents() const -> glm::vec3
  {
    return (Max - Min) * 0.5F;
  }

  [[nodiscard]] auto GetSize() const -> glm::vec3 { return Max - Min; }

  void Expand(const glm::vec3& point)
  {
    Min = glm::min(Min, point);
    Max = glm::max(Max, point);
  }

  void Expand(const AABB& other)
  {
    if (other.IsValid()) {
      Min = glm::min(Min, other.Min);
      Max = glm::max(Max, other.Max);
    }
  }

  [[nodiscard]] auto IsValid() const -> bool
  {
    return Max.x >= Min.x && Max.y >= Min.y && Max.z >= Min.z;
  }

  void Reset()
  {
    Min = glm::vec3(std::numeric_limits<float>::max());
    Max = glm::vec3(std::numeric_limits<float>::lowest());
  }

  // Transform AABB by matrix (returns new, potentially larger AABB)
  [[nodiscard]] auto Transform(const glm::mat4& matrix) const -> AABB
  {
    if (!IsValid()) {
      return {};
    }

    // Get all 8 corners of the AABB
    std::array<glm::vec3, 8> corners = {
        glm::vec3 {Min.x, Min.y, Min.z},
        glm::vec3 {Max.x, Min.y, Min.z},
        glm::vec3 {Min.x, Max.y, Min.z},
        glm::vec3 {Max.x, Max.y, Min.z},
        glm::vec3 {Min.x, Min.y, Max.z},
        glm::vec3 {Max.x, Min.y, Max.z},
        glm::vec3 {Min.x, Max.y, Max.z},
        glm::vec3 {Max.x, Max.y, Max.z},
    };

    AABB result {};
    for (const auto& corner : corners) {
      glm::vec4 transformed = matrix * glm::vec4(corner, 1.0F);
      result.Expand(glm::vec3(transformed));
    }

    return result;
  }

  static auto CreateFromPoints(const std::span<glm::vec3> points) -> AABB
  {
    AABB result {};
    for (auto point : points) {
      result.Expand(point);
    }
    return result;
  }
};

// Bounding Sphere (for quick culling checks)
struct BoundingSphere
{
  glm::vec3 Center {0.0F, 0.0F, 0.0F};
  float Radius {0.0F};

  [[nodiscard]] auto IsValid() const -> bool { return Radius > 0.0F; }

  [[nodiscard]] static auto FromAABB(const AABB& aabb) -> BoundingSphere
  {
    if (!aabb.IsValid()) {
      return {};
    }

    BoundingSphere sphere {};
    sphere.Center = aabb.GetCenter();
    sphere.Radius = glm::length(aabb.GetExtents());
    return sphere;
  }
};

#endif
