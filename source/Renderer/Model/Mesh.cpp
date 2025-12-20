#include "Renderer/Model/Mesh.hpp"

#include "Renderer/RHI/RHIBuffer.hpp"
#include "Renderer/RHI/RHIDevice.hpp"

uint64_t Mesh::SNextId = 1;

Mesh::Mesh()
    : m_VertexLayout(Vertex::GetLayout())
    , m_Id(SNextId++)
{
}

Mesh::Mesh(std::string name)
    : m_Name(std::move(name))
    , m_VertexLayout(Vertex::GetLayout())
    , m_Id(SNextId++)
{
}

Mesh::Mesh(Mesh&&) noexcept = default;
auto Mesh::operator=(Mesh&&) noexcept -> Mesh& = default;
Mesh::~Mesh() = default;

void Mesh::SetVertices(std::vector<Vertex> vertices)
{
  m_Vertices = std::move(vertices);
  ComputeBounds();
}

void Mesh::SetIndices(std::vector<uint32_t> indices)
{
  m_Indices = std::move(indices);
}

void Mesh::AddSubMesh(const SubMesh& submesh)
{
  m_SubMeshes.push_back(submesh);
}

void Mesh::AddSubMesh(uint32_t index_offset,
                      uint32_t index_count,
                      uint32_t material_index)
{
  SubMesh submesh {};
  submesh.IndexOffset = index_offset;
  submesh.IndexCount = index_count;
  submesh.VertexOffset = 0;
  submesh.MaterialIndex = material_index;
  submesh.LocalBounds = m_Bounds;  // TODO: compute per-submesh bounds
  m_SubMeshes.push_back(submesh);
}

void Mesh::CreateSingleSubMesh(uint32_t material_index)
{
  m_SubMeshes.clear();

  SubMesh submesh {};
  submesh.IndexOffset = 0;
  submesh.IndexCount = static_cast<uint32_t>(m_Indices.size());
  submesh.VertexOffset = 0;
  submesh.MaterialIndex = material_index;
  submesh.LocalBounds = m_Bounds;
  m_SubMeshes.push_back(submesh);
}

void Mesh::CreateBuffers(RHIDevice& device)
{
  if (m_BuffersCreated || m_Vertices.empty()) {
    return;
  }

  // Create vertex buffer
  BufferDesc vertex_buffer_desc {};
  vertex_buffer_desc.Size = sizeof(Vertex) * m_Vertices.size();
  vertex_buffer_desc.Usage = BufferUsage::Vertex;
  vertex_buffer_desc.CPUVisible = true;
  m_VertexBuffer = device.CreateBuffer(vertex_buffer_desc);
  m_VertexBuffer->Upload(m_Vertices.data(), vertex_buffer_desc.Size, 0);

  // Create index buffer if we have indices
  if (!m_Indices.empty()) {
    BufferDesc index_buffer_desc {};
    index_buffer_desc.Size = sizeof(uint32_t) * m_Indices.size();
    index_buffer_desc.Usage = BufferUsage::Index;
    index_buffer_desc.CPUVisible = true;
    m_IndexBuffer = device.CreateBuffer(index_buffer_desc);
    m_IndexBuffer->Upload(m_Indices.data(), index_buffer_desc.Size, 0);
  }

  m_BuffersCreated = true;
}

void Mesh::DestroyBuffers()
{
  m_VertexBuffer.reset();
  m_IndexBuffer.reset();
  m_BuffersCreated = false;
}

auto Mesh::GetName() const -> const std::string&
{
  return m_Name;
}

auto Mesh::GetVertexBuffer() const -> RHIBuffer*
{
  return m_VertexBuffer.get();
}

auto Mesh::GetIndexBuffer() const -> RHIBuffer*
{
  return m_IndexBuffer.get();
}

auto Mesh::GetVertexLayout() const -> const VertexInputLayout&
{
  return m_VertexLayout;
}

auto Mesh::GetSubMeshes() const -> const std::vector<SubMesh>&
{
  return m_SubMeshes;
}

auto Mesh::GetSubMesh(size_t index) const -> const SubMesh&
{
  return m_SubMeshes.at(index);
}

auto Mesh::GetSubMeshCount() const -> size_t
{
  return m_SubMeshes.size();
}

auto Mesh::GetBounds() const -> const AABB&
{
  return m_Bounds;
}

auto Mesh::GetVertexCount() const -> uint32_t
{
  return static_cast<uint32_t>(m_Vertices.size());
}

auto Mesh::GetIndexCount() const -> uint32_t
{
  return static_cast<uint32_t>(m_Indices.size());
}

auto Mesh::AreBuffersCreated() const -> bool
{
  return m_BuffersCreated;
}

auto Mesh::GetVertices() const -> const std::vector<Vertex>&
{
  return m_Vertices;
}

auto Mesh::GetIndices() const -> const std::vector<uint32_t>&
{
  return m_Indices;
}

void Mesh::ComputeBounds()
{
  m_Bounds.Reset();
  for (const auto& vertex : m_Vertices) {
    m_Bounds.Expand(vertex.Position);
  }

  // Update submesh bounds as well
  for (auto& submesh : m_SubMeshes) {
    submesh.LocalBounds = m_Bounds;
  }
}

void Mesh::ComputeTangents()
{
  if (!m_Indices.empty() && !m_Vertices.empty()) {
    ::ComputeTangents(m_Vertices, m_Indices);
  }
}

auto Mesh::GetId() const -> uint64_t
{
  return m_Id;
}
