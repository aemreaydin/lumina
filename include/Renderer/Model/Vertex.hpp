#ifndef RENDERER_MODEL_VERTEX_HPP
#define RENDERER_MODEL_VERTEX_HPP

#include <cstdint>
#include <vector>

#include <glm/glm.hpp>

#include "Renderer/RHI/RHIVertexLayout.hpp"

// Vertex attribute flags for flexible vertex formats
enum class VertexAttributeFlags : uint8_t
{
  None = 0,
  Position = 1 << 0,
  Normal = 1 << 1,
  TexCoord = 1 << 2,
  Tangent = 1 << 3,
  Color = 1 << 4,
};

constexpr auto operator|(VertexAttributeFlags lhs, VertexAttributeFlags rhs)
    -> VertexAttributeFlags
{
  return static_cast<VertexAttributeFlags>(static_cast<uint32_t>(lhs)
                                           | static_cast<uint32_t>(rhs));
}

constexpr auto operator&(VertexAttributeFlags lhs, VertexAttributeFlags rhs)
    -> VertexAttributeFlags
{
  return static_cast<VertexAttributeFlags>(static_cast<uint32_t>(lhs)
                                           & static_cast<uint32_t>(rhs));
}

constexpr auto operator~(VertexAttributeFlags flags) -> VertexAttributeFlags
{
  return static_cast<VertexAttributeFlags>(~static_cast<uint32_t>(flags));
}

[[nodiscard]] constexpr auto HasFlag(VertexAttributeFlags flags,
                                     VertexAttributeFlags flag) -> bool
{
  return (static_cast<uint32_t>(flags) & static_cast<uint32_t>(flag)) != 0;
}

// Primary vertex format with tangents for normal mapping
struct Vertex
{
  glm::vec3 Position {0.0F, 0.0F, 0.0F};
  glm::vec3 Normal {0.0F, 0.0F, 1.0F};
  glm::vec2 TexCoord {0.0F, 0.0F};
  glm::vec4 Tangent {1.0F, 0.0F, 0.0F, 1.0F};  // w = handedness (+1 or -1)

  static constexpr VertexAttributeFlags FLAGS = VertexAttributeFlags::Position
      | VertexAttributeFlags::Normal | VertexAttributeFlags::TexCoord
      | VertexAttributeFlags::Tangent;

  [[nodiscard]] static auto GetLayout() -> VertexInputLayout
  {
    VertexInputLayout layout {};
    layout.Stride = sizeof(Vertex);
    layout.Attributes = {
        {0, VertexFormat::Float3, offsetof(Vertex, Position)},
        {1, VertexFormat::Float3, offsetof(Vertex, Normal)},
        {2, VertexFormat::Float2, offsetof(Vertex, TexCoord)},
        {3, VertexFormat::Float4, offsetof(Vertex, Tangent)},
    };
    return layout;
  }

  auto operator==(const Vertex& other) const -> bool
  {
    return Position == other.Position && Normal == other.Normal
        && TexCoord == other.TexCoord && Tangent == other.Tangent;
  }
};

void ComputeTangents(std::vector<Vertex>& vertices,
                     const std::vector<uint32_t>& indices);

#endif
