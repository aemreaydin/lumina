#include <cmath>

#include "Renderer/Model/Vertex.hpp"

#include <glm/geometric.hpp>

void ComputeTangents(std::vector<Vertex>& vertices,
                     const std::vector<uint32_t>& indices)
{
  if (indices.size() < 3 || vertices.empty()) {
    return;
  }

  // Accumulate tangent and bitangent for each vertex
  std::vector<glm::vec3> tangents(vertices.size(), glm::vec3(0.0F));
  std::vector<glm::vec3> bitangents(vertices.size(), glm::vec3(0.0F));

  // Process each triangle
  for (size_t i = 0; i + 2 < indices.size(); i += 3) {
    uint32_t idx0 = indices[i];
    uint32_t idx1 = indices[i + 1];
    uint32_t idx2 = indices[i + 2];

    if (idx0 >= vertices.size() || idx1 >= vertices.size()
        || idx2 >= vertices.size())
    {
      continue;
    }

    const Vertex& vert0 = vertices[idx0];
    const Vertex& vert1 = vertices[idx1];
    const Vertex& vert2 = vertices[idx2];

    // Edge vectors
    glm::vec3 edge1 = vert1.Position - vert0.Position;
    glm::vec3 edge2 = vert2.Position - vert0.Position;

    // UV delta
    glm::vec2 delta_uv1 = vert1.TexCoord - vert0.TexCoord;
    glm::vec2 delta_uv2 = vert2.TexCoord - vert0.TexCoord;

    // Calculate determinant
    float denom = (delta_uv1.x * delta_uv2.y) - (delta_uv2.x * delta_uv1.y);

    if (std::fabs(denom) < 1e-8F) {
      continue;
    }

    float inv_denom = 1.0F / denom;

    // Calculate tangent and bitangent
    glm::vec3 tangent = inv_denom * (delta_uv2.y * edge1 - delta_uv1.y * edge2);
    glm::vec3 bitangent =
        inv_denom * (-delta_uv2.x * edge1 + delta_uv1.x * edge2);

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
    const glm::vec3& normal = vertices[i].Normal;
    const glm::vec3& tangent = tangents[i];
    const glm::vec3& bitangent = bitangents[i];

    // Skip if no tangent data
    if (glm::length(tangent) < 1e-8F) {
      // Use a default tangent perpendicular to normal
      glm::vec3 default_tangent;
      if (std::abs(normal.x) < 0.9F) {
        default_tangent =
            glm::normalize(glm::cross(normal, glm::vec3(1, 0, 0)));
      } else {
        default_tangent =
            glm::normalize(glm::cross(normal, glm::vec3(0, 1, 0)));
      }
      vertices[i].Tangent = glm::vec4(default_tangent, 1.0F);
      continue;
    }

    // Gram-Schmidt orthogonalize: T' = normalize(T - N * dot(N, T))
    glm::vec3 ortho_tangent =
        glm::normalize(tangent - normal * glm::dot(normal, tangent));

    // Calculate handedness: sign of dot(cross(N, T), B)
    float handedness = (glm::dot(glm::cross(normal, tangent), bitangent) < 0.0F)
        ? -1.0F
        : 1.0F;

    vertices[i].Tangent = glm::vec4(ortho_tangent, handedness);
  }
}
