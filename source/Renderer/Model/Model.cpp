#include "Renderer/Model/Model.hpp"

#include "Renderer/Model/Material.hpp"
#include "Renderer/Model/Mesh.hpp"
#include "Renderer/RHI/RHIDevice.hpp"

Model::Model(std::string name)
    : m_Name(std::move(name))
{
}

Model::Model(Model&&) noexcept = default;
auto Model::operator=(Model&&) noexcept -> Model& = default;
Model::~Model() = default;

void Model::AddMesh(std::unique_ptr<Mesh> mesh)
{
  if (mesh) {
    m_Meshes.push_back(std::move(mesh));
    compute_bounds();
  }
}

void Model::AddMaterial(std::unique_ptr<Material> material)
{
  if (material) {
    m_Materials.push_back(std::move(material));
  }
}

void Model::CreateResources(
    RHIDevice& device,
    const std::shared_ptr<RHIDescriptorSetLayout>& material_layout,
    RHISampler* default_sampler,
    RHITexture* default_texture,
    RHITexture* default_normal)
{
  if (m_ResourcesCreated) {
    return;
  }

  // Create mesh buffers
  for (auto& mesh : m_Meshes) {
    mesh->CreateBuffers(device);
  }

  // Create material descriptor sets
  for (auto& material : m_Materials) {
    material->CreateDescriptorSet(device,
                                  material_layout,
                                  default_sampler,
                                  default_texture,
                                  default_normal);
  }

  m_ResourcesCreated = true;
}

void Model::DestroyResources()
{
  for (auto& mesh : m_Meshes) {
    mesh->DestroyBuffers();
  }
  m_ResourcesCreated = false;
}

auto Model::GetName() const -> const std::string&
{
  return m_Name;
}

auto Model::GetMeshes() const -> const std::vector<std::unique_ptr<Mesh>>&
{
  return m_Meshes;
}

auto Model::GetMesh(size_t index) const -> Mesh*
{
  if (index < m_Meshes.size()) {
    return m_Meshes[index].get();
  }
  return nullptr;
}

auto Model::GetMeshCount() const -> size_t
{
  return m_Meshes.size();
}

auto Model::GetMaterials() const
    -> const std::vector<std::unique_ptr<Material>>&
{
  return m_Materials;
}

auto Model::GetMaterial(size_t index) const -> Material*
{
  if (index < m_Materials.size()) {
    return m_Materials[index].get();
  }
  return nullptr;
}

auto Model::GetMaterialCount() const -> size_t
{
  return m_Materials.size();
}

auto Model::GetBounds() const -> const AABB&
{
  return m_Bounds;
}

auto Model::AreResourcesCreated() const -> bool
{
  return m_ResourcesCreated;
}

void Model::SetSourcePath(const std::string& path)
{
  m_SourcePath = path;
}

auto Model::GetSourcePath() const -> const std::string&
{
  return m_SourcePath;
}

void Model::compute_bounds()
{
  m_Bounds.Reset();
  for (const auto& mesh : m_Meshes) {
    m_Bounds.Expand(mesh->GetBounds());
  }
}
