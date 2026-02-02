#include <cmath>

#include "Renderer/Model/Vertex.hpp"

void ComputeTangents(std::vector<Vertex>& vertices,
                     const std::vector<uint32_t>& indices)
{
  if (indices.size() < 3 || vertices.empty()) {
    return;
  }

  // Accumulate tangent and bitangent for each vertex
  std::vector<linalg::Vec3> tangents(vertices.size(), linalg::Vec3{0.0F, 0.0F, 0.0F});
  std::vector<linalg::Vec3> bitangents(vertices.size(), linalg::Vec3{0.0F, 0.0F, 0.0F});

  // Process each triangle
  for (size_t i = 0; i + 2 < indices.size(); i += 3) {
    const uint32_t idx0 = indices[i];
    const uint32_t idx1 = indices[i + 1];
    const uint32_t idx2 = indices[i + 2];

    if (idx0 >= vertices.size() || idx1 >= vertices.size()
        || idx2 >= vertices.size())
    {
      continue;
    }

    const Vertex& vert0 = vertices[idx0];
    const Vertex& vert1 = vertices[idx1];
    const Vertex& vert2 = vertices[idx2];

    // Edge vectors
    const linalg::Vec3 edge1 = vert1.Position - vert0.Position;
    const linalg::Vec3 edge2 = vert2.Position - vert0.Position;

    // UV delta
    const linalg::Vec2 delta_uv1 = vert1.TexCoord - vert0.TexCoord;
    const linalg::Vec2 delta_uv2 = vert2.TexCoord - vert0.TexCoord;

    const float denom =
        (delta_uv1.x() * delta_uv2.y()) - (delta_uv2.x() * delta_uv1.y());

    if (std::fabs(denom) < 1e-8F) {
      continue;
    }

    const float inv_denom = 1.0F / denom;

    // Calculate tangent and bitangent
    const linalg::Vec3 tangent =
        inv_denom * (delta_uv2.y() * edge1 - delta_uv1.y() * edge2);
    const linalg::Vec3 bitangent =
        inv_denom * (-delta_uv2.x() * edge1 + delta_uv1.x() * edge2);

    // Accumulate for each vertex of the triangle
    tangents[idx0] += tangent;
    tangents[idx1] += tangent;
    tangents[idx2] += tangent;

    bitangents[idx0] += bitangent;
    bitangents[idx1] += bitangent;
    bitangents[idx2] += bitangent;
  }

  // Orthonormalize and compute handedness for each vertex
  for (size_t i = 0; i < vertices.size(); ++i) {
    const linalg::Vec3& normal = vertices[i].Normal;
    const linalg::Vec3& tangent = tangents[i];
    const linalg::Vec3& bitangent = bitangents[i];

    // Skip if no tangent data
    if (linalg::magnitude(tangent) < 1e-8F) {
      // Use a default tangent perpendicular to normal
      linalg::Vec3 default_tangent;
      if (std::abs(normal.x()) < 0.9F) {
        default_tangent =
            linalg::normalized(linalg::cross(normal, linalg::Vec3{1.0F, 0.0F, 0.0F}));
      } else {
        default_tangent =
            linalg::normalized(linalg::cross(normal, linalg::Vec3{0.0F, 1.0F, 0.0F}));
      }
      vertices[i].Tangent = linalg::Vec4(default_tangent, 1.0F);
      continue;
    }

    // Gram-Schmidt orthogonalize: T' = normalize(T - N * dot(N, T))
    const linalg::Vec3 ortho_tangent =
        linalg::normalized(tangent - normal * linalg::dot(normal, tangent));

    // Calculate handedness: sign of dot(cross(N, T), B)
    const float handedness =
        (linalg::dot(linalg::cross(normal, tangent), bitangent) < 0.0F) ? -1.0F
                                                                  : 1.0F;

    vertices[i].Tangent = linalg::Vec4(ortho_tangent, handedness);
  }
}
