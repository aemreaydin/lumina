#ifndef RENDERER_MODEL_BOUNDINGVOLUME_HPP
#define RENDERER_MODEL_BOUNDINGVOLUME_HPP

#include <algorithm>
#include <array>
#include <limits>
#include <span>

#include <linalg/vec.hpp>
#include <linalg/mat4.hpp>
#include <linalg/utility.hpp>

struct Ray
{
  linalg::Vec3 Origin {0.0F, 0.0F, 0.0F};
  linalg::Vec3 Direction {0.0F, 1.0F, 0.0F};
};

// Axis-Aligned Bounding Box
struct AABB
{
  linalg::Vec3 Min {std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};
  linalg::Vec3 Max {std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest()};

  [[nodiscard]] auto GetCenter() const -> linalg::Vec3
  {
    return (Min + Max) * 0.5F;
  }

  [[nodiscard]] auto GetExtents() const -> linalg::Vec3
  {
    return (Max - Min) * 0.5F;
  }

  [[nodiscard]] auto GetSize() const -> linalg::Vec3 { return Max - Min; }

  void Expand(const linalg::Vec3& point)
  {
    Min = linalg::min(Min, point);
    Max = linalg::max(Max, point);
  }

  void Expand(const AABB& other)
  {
    if (other.IsValid()) {
      Min = linalg::min(Min, other.Min);
      Max = linalg::max(Max, other.Max);
    }
  }

  [[nodiscard]] auto IsValid() const -> bool
  {
    return Max.x() >= Min.x() && Max.y() >= Min.y() && Max.z() >= Min.z();
  }

  void Reset()
  {
    Min = linalg::Vec3{std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};
    Max = linalg::Vec3{std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest()};
  }

  // Transform AABB by matrix (returns new, potentially larger AABB)
  [[nodiscard]] auto Transform(const linalg::Mat4& matrix) const -> AABB
  {
    if (!IsValid()) {
      return {};
    }

    // Get all 8 corners of the AABB
    const std::array<linalg::Vec3, 8> corners = {
        linalg::Vec3 {Min.x(), Min.y(), Min.z()},
        linalg::Vec3 {Max.x(), Min.y(), Min.z()},
        linalg::Vec3 {Min.x(), Max.y(), Min.z()},
        linalg::Vec3 {Max.x(), Max.y(), Min.z()},
        linalg::Vec3 {Min.x(), Min.y(), Max.z()},
        linalg::Vec3 {Max.x(), Min.y(), Max.z()},
        linalg::Vec3 {Min.x(), Max.y(), Max.z()},
        linalg::Vec3 {Max.x(), Max.y(), Max.z()},
    };

    AABB result {};
    for (const auto& corner : corners) {
      const linalg::Vec4 transformed = matrix * linalg::Vec4(corner, 1.0F);
      result.Expand(transformed.to_sub_vec<3>());
    }

    return result;
  }

  static auto CreateFromPoints(const std::span<linalg::Vec3> points) -> AABB
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

    const linalg::Vec3 inv_dir = 1.0F / ray.Direction;

    const linalg::Vec3 t_min = (Min - ray.Origin) * inv_dir;
    const linalg::Vec3 t_max = (Max - ray.Origin) * inv_dir;

    const linalg::Vec3 t1 = linalg::min(t_min, t_max);
    const linalg::Vec3 t2 = linalg::max(t_min, t_max);

    const float t_near = std::max({t1.x(), t1.y(), t1.z()});
    const float t_far = std::min({t2.x(), t2.y(), t2.z()});

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
  linalg::Vec3 Center {0.0F, 0.0F, 0.0F};
  float Radius {0.0F};

  [[nodiscard]] auto IsValid() const -> bool { return Radius > 0.0F; }

  [[nodiscard]] static auto FromAABB(const AABB& aabb) -> BoundingSphere
  {
    if (!aabb.IsValid()) {
      return {};
    }

    BoundingSphere sphere {};
    sphere.Center = aabb.GetCenter();
    sphere.Radius = linalg::magnitude(aabb.GetExtents());
    return sphere;
  }
};

#endif
