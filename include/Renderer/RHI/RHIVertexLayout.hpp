#ifndef RENDERER_RHI_RHIVERTEXLAYOUT_HPP
#define RENDERER_RHI_RHIVERTEXLAYOUT_HPP

#include <cstdint>
#include <vector>

enum class VertexFormat : uint8_t
{
  Float,
  Float2,
  Float3,
  Float4,
  Int,
  Int2,
  Int3,
  Int4,
  UInt,
  UInt2,
  UInt3,
  UInt4
};

enum class PrimitiveTopology : uint8_t
{
  TriangleList,
  TriangleStrip,
  LineList,
  LineStrip,
  PointList
};

enum class PolygonMode : uint8_t
{
  Fill,
  Line,
  Point
};

enum class IndexType : uint8_t
{
  UInt16,
  UInt32
};

struct VertexAttribute
{
  uint32_t Location {0};
  VertexFormat Format {VertexFormat::Float3};
  uint32_t Offset {0};
};

struct VertexInputLayout
{
  uint32_t Stride {0};
  std::vector<VertexAttribute> Attributes;
};

#endif
