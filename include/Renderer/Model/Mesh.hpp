#ifndef RENDERER_MODEL_MESH_HPP
#define RENDERER_MODEL_MESH_HPP

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "Renderer/Model/BoundingVolume.hpp"
#include "Renderer/Model/Vertex.hpp"
#include "Renderer/RHI/RHIVertexLayout.hpp"

class RHIBuffer;
class RHIDevice;

struct SubMesh
{
  uint32_t IndexOffset {0};
  uint32_t IndexCount {0};
  uint32_t VertexOffset {0};
  uint32_t MaterialIndex {0};
  AABB LocalBounds {};
};

class Mesh
{
public:
  Mesh();
  explicit Mesh(std::string name);

  Mesh(const Mesh&) = delete;
  Mesh(Mesh&&) noexcept;
  auto operator=(const Mesh&) -> Mesh& = delete;
  auto operator=(Mesh&&) noexcept -> Mesh&;
  ~Mesh();

  // Set vertex and index data
  void SetVertices(std::vector<Vertex> vertices);
  void SetIndices(std::vector<uint32_t> indices);

  // Add submesh with explicit parameters
  void AddSubMesh(const SubMesh& submesh);
  void AddSubMesh(uint32_t index_offset,
                  uint32_t index_count,
                  uint32_t material_index);

  // Create a single submesh covering all indices (clears existing submeshes)
  void CreateSingleSubMesh(uint32_t material_index = 0);

  // Create GPU buffers (must be called before rendering)
  void CreateBuffers(RHIDevice& device);
  void DestroyBuffers();

  // Getters
  [[nodiscard]] auto GetName() const -> const std::string&;
  [[nodiscard]] auto GetVertexBuffer() const -> RHIBuffer*;
  [[nodiscard]] auto GetIndexBuffer() const -> RHIBuffer*;
  [[nodiscard]] auto GetVertexLayout() const -> const VertexInputLayout&;
  [[nodiscard]] auto GetSubMeshes() const -> const std::vector<SubMesh>&;
  [[nodiscard]] auto GetSubMesh(size_t index) const -> const SubMesh&;
  [[nodiscard]] auto GetSubMeshCount() const -> size_t;
  [[nodiscard]] auto GetBounds() const -> const AABB&;
  [[nodiscard]] auto GetVertexCount() const -> uint32_t;
  [[nodiscard]] auto GetIndexCount() const -> uint32_t;
  [[nodiscard]] auto AreBuffersCreated() const -> bool;

  // Access to CPU-side data (available before CreateBuffers or if retained)
  [[nodiscard]] auto GetVertices() const -> const std::vector<Vertex>&;
  [[nodiscard]] auto GetIndices() const -> const std::vector<uint32_t>&;

  // Compute mesh bounds from vertex data
  void ComputeBounds();

  // Compute tangents if not already present
  void ComputeTangents();

  // Unique ID for sorting/hashing
  [[nodiscard]] auto GetId() const -> uint64_t;

private:
  std::string m_Name {"Unnamed"};

  std::vector<Vertex> m_Vertices;
  std::vector<uint32_t> m_Indices;
  std::vector<SubMesh> m_SubMeshes;

  VertexInputLayout m_VertexLayout;
  AABB m_Bounds {};

  std::unique_ptr<RHIBuffer> m_VertexBuffer;
  std::unique_ptr<RHIBuffer> m_IndexBuffer;

  uint64_t m_Id {0};
  bool m_BuffersCreated {false};

  static uint64_t SNextId;
};

#endif
