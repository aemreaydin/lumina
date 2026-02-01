#ifndef RENDERER_MODEL_BOUNDINGVOLUME_HPP
#define RENDERER_MODEL_BOUNDINGVOLUME_HPP

#include <algorithm>
#include <array>
#include <limits>
#include <span>

#include <glm/fwd.hpp>
#include <glm/glm.hpp>

struct Ray
{
  glm::vec3 Origin {0.0F};
  glm::vec3 Direction {0.0F, 1.0F, 0.0F};
};

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
    const std::array<glm::vec3, 8> corners = {
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
      const glm::vec4 transformed = matrix * glm::vec4(corner, 1.0F);
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

  // Ray-AABB intersection using the slab method.
  // Returns true if the ray hits, writing the entry distance to t_hit.
  [[nodiscard]] auto Intersects(const Ray& ray, float& t_hit) const -> bool
  {
    if (!IsValid()) {
      return false;
    }

    const glm::vec3 inv_dir = 1.0F / ray.Direction;

    const glm::vec3 t_min = (Min - ray.Origin) * inv_dir;
    const glm::vec3 t_max = (Max - ray.Origin) * inv_dir;

    const glm::vec3 t1 = glm::min(t_min, t_max);
    const glm::vec3 t2 = glm::max(t_min, t_max);

    const float t_near = std::max({t1.x, t1.y, t1.z});
    const float t_far = std::min({t2.x, t2.y, t2.z});

    if (t_near > t_far || t_far < 0.0F) {
      return false;
    }

    t_hit = t_near >= 0.0F ? t_near : t_far;
    return true;
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
