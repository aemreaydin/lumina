#include <algorithm>
#include <map>
#include <ranges>
#include <unordered_map>

#include "Renderer/Model/ModelLoader.hpp"

#include <tiny_obj_loader.h>

#include "Core/Logger.hpp"
#include "Renderer/Asset/AssetManager.hpp"
#include "Renderer/Model/Material.hpp"
#include "Renderer/Model/Mesh.hpp"
#include "Renderer/Model/Model.hpp"
#include "Renderer/Model/Vertex.hpp"

struct VertexHash
{
  auto operator()(const Vertex& vertex) const -> size_t
  {
    size_t seed = 0;
    auto hash_combine = [&seed](auto value) -> auto
    {
      seed ^= std::hash<decltype(value)> {}(value) + 0x9e3779b9 + (seed << 6)
          + (seed >> 2);
    };

    hash_combine(vertex.Position.x);
    hash_combine(vertex.Position.y);
    hash_combine(vertex.Position.z);
    hash_combine(vertex.Normal.x);
    hash_combine(vertex.Normal.y);
    hash_combine(vertex.Normal.z);
    hash_combine(vertex.TexCoord.x);
    hash_combine(vertex.TexCoord.y);

    return seed;
  }
};

static auto NormalizeTexturePath(const std::filesystem::path& model_path,
                                 const std::string& tex_name) -> std::string
{
  std::string tex_path =
      std::filesystem::absolute(model_path.parent_path() / tex_name);
  std::ranges::replace(tex_path, '\\', '/');
  return tex_path;
}

static auto LoadMaterialTexture(AssetManager& asset_manager,
                                const std::filesystem::path& model_path,
                                const std::string& tex_name,
                                bool is_srgb) -> std::shared_ptr<RHITexture>
{
  if (tex_name.empty()) {
    return nullptr;
  }

  const auto tex_path = NormalizeTexturePath(model_path, tex_name);
  TextureLoadOptions tex_options {};
  tex_options.SRGB = is_srgb;
  return asset_manager.LoadTexture(tex_path, tex_options);
}

static auto CreateMaterialFromTinyobj(const tinyobj::material_t& mat,
                                      const std::filesystem::path& model_path,
                                      AssetManager& asset_manager)
    -> std::unique_ptr<Material>
{
  auto material = std::make_unique<Material>(mat.name);

  // Set base color from diffuse
  material->SetBaseColorFactor(
      glm::vec4(mat.diffuse[0], mat.diffuse[1], mat.diffuse[2], 1.0F));

  // Estimate metallic/roughness from specular
  const float specular_avg =
      (mat.specular[0] + mat.specular[1] + mat.specular[2]) / 3.0F;
  material->SetMetallicFactor(specular_avg > 0.5F ? 0.5F : 0.0F);
  material->SetRoughnessFactor(1.0F - (mat.shininess / 1000.0F));

  // Load diffuse texture
  if (auto texture = LoadMaterialTexture(
          asset_manager, model_path, mat.diffuse_texname, /*is_srgb=*/true))
  {
    material->SetBaseColorTexture(texture);
  }

  // Load normal map
  if (auto texture = LoadMaterialTexture(
          asset_manager, model_path, mat.bump_texname, /*is_srgb=*/false))
  {
    material->SetNormalTexture(texture);
  }

  return material;
}

static auto ExtractVertex(const tinyobj::attrib_t& attrib,
                          const tinyobj::index_t& idx,
                          const ModelLoadOptions& options) -> Vertex
{
  Vertex vertex {};

  // Position
  vertex.Position = {
      attrib.vertices[(3 * static_cast<size_t>(idx.vertex_index)) + 0]
          * options.Scale,
      attrib.vertices[(3 * static_cast<size_t>(idx.vertex_index)) + 1]
          * options.Scale,
      attrib.vertices[(3 * static_cast<size_t>(idx.vertex_index)) + 2]
          * options.Scale,
  };

  // Normal
  if (idx.normal_index >= 0 && !attrib.normals.empty()) {
    vertex.Normal = {
        attrib.normals[(3 * static_cast<size_t>(idx.normal_index)) + 0],
        attrib.normals[(3 * static_cast<size_t>(idx.normal_index)) + 1],
        attrib.normals[(3 * static_cast<size_t>(idx.normal_index)) + 2],
    };
  }

  // TexCoord
  if (idx.texcoord_index >= 0 && !attrib.texcoords.empty()) {
    const float tex_u =
        attrib.texcoords[(2 * static_cast<size_t>(idx.texcoord_index)) + 0];
    const float tex_v =
        attrib.texcoords[(2 * static_cast<size_t>(idx.texcoord_index)) + 1];
    vertex.TexCoord = {tex_u, options.FlipUVs ? (1.0F - tex_v) : tex_v};
  }

  return vertex;
}

static auto ProcessShape(const tinyobj::shape_t& shape,
                         const tinyobj::attrib_t& attrib,
                         const ModelLoadOptions& options,
                         size_t material_count) -> std::unique_ptr<Mesh>
{
  auto mesh = std::make_unique<Mesh>(shape.name);

  std::vector<Vertex> vertices;
  std::unordered_map<Vertex, uint32_t, VertexHash> unique_vertices;
  std::map<int, std::vector<uint32_t>> indices_by_material;

  size_t index_offset = 0;
  for (size_t face_idx = 0; face_idx < shape.mesh.num_face_vertices.size();
       ++face_idx)
  {
    const size_t num_verts = shape.mesh.num_face_vertices[face_idx];
    int material_id =
        shape.mesh.material_ids.empty() ? 0 : shape.mesh.material_ids[face_idx];
    material_id = std::max(material_id, 0);

    for (size_t vert = 0; vert < num_verts; ++vert) {
      const auto& idx = shape.mesh.indices[index_offset + vert];
      const Vertex vertex = ExtractVertex(attrib, idx, options);

      if (!unique_vertices.contains(vertex)) {
        unique_vertices[vertex] = static_cast<uint32_t>(vertices.size());
        vertices.push_back(vertex);
      }

      indices_by_material[material_id].push_back(unique_vertices[vertex]);
    }

    index_offset += num_verts;
  }

  // Combine indices in material order
  std::vector<uint32_t> indices;
  std::vector<std::tuple<uint32_t, uint32_t, int>> submesh_ranges;

  for (const auto& [mat_id, mat_indices] : indices_by_material) {
    auto start_offset = static_cast<uint32_t>(indices.size());
    auto count = static_cast<uint32_t>(mat_indices.size());
    indices.insert(indices.end(), mat_indices.begin(), mat_indices.end());
    submesh_ranges.emplace_back(start_offset, count, mat_id);
  }

  mesh->SetVertices(std::move(vertices));
  mesh->SetIndices(std::move(indices));

  if (options.CalculateTangents) {
    mesh->ComputeTangents();
  }

  // Add submeshes
  for (const auto& [start_offset, count, mat_id] : submesh_ranges) {
    auto mat_index = static_cast<uint32_t>(mat_id);
    if (mat_index >= material_count) {
      mat_index = 0;
    }
    mesh->AddSubMesh(start_offset, count, mat_index);
  }

  if (mesh->GetSubMeshCount() == 0) {
    mesh->CreateSingleSubMesh(0);
  }

  return mesh;
}

auto OBJModelLoader::CanLoad(const std::filesystem::path& path) const -> bool
{
  const auto extension = path.extension().string();
  return extension == ".obj" || extension == ".OBJ";
}

auto OBJModelLoader::Load(const std::filesystem::path& path,
                          AssetManager& asset_manager,
                          const ModelLoadOptions& options)
    -> std::unique_ptr<Model>
{
  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;
  std::string warning;
  std::string error;

  std::string base_dir = path.parent_path().string();
  if (!base_dir.empty()) {
    base_dir += "/";
  }

  const bool success = tinyobj::LoadObj(&attrib,
                                        &shapes,
                                        &materials,
                                        &warning,
                                        &error,
                                        path.string().c_str(),
                                        base_dir.c_str());

  if (!warning.empty()) {
    Logger::Warn("OBJ loader warning: {}", warning);
  }

  if (!error.empty()) {
    Logger::Error("OBJ loader error: {}", error);
  }

  if (!success) {
    return nullptr;
  }

  auto model = std::make_unique<Model>(path.stem().string());

  // Load materials
  for (const auto& mat : materials) {
    model->AddMaterial(CreateMaterialFromTinyobj(mat, path, asset_manager));
  }

  // Add default material if none were loaded
  if (model->GetMaterialCount() == 0) {
    auto default_material = std::make_unique<Material>("Default");
    default_material->SetBaseColorFactor(glm::vec4(0.8F, 0.8F, 0.8F, 1.0F));
    model->AddMaterial(std::move(default_material));
  }

  // Process shapes into meshes
  for (const auto& shape : shapes) {
    model->AddMesh(
        ProcessShape(shape, attrib, options, model->GetMaterialCount()));
  }

  return model;
}

auto ModelLoaderRegistry::Instance() -> ModelLoaderRegistry&
{
  static ModelLoaderRegistry Instance;
  return Instance;
}

ModelLoaderRegistry::ModelLoaderRegistry()
{
  RegisterLoader(std::make_unique<OBJModelLoader>());
}

void ModelLoaderRegistry::RegisterLoader(std::unique_ptr<IModelLoader> loader)
{
  m_Loaders.push_back(std::move(loader));
}

auto ModelLoaderRegistry::GetLoaderFor(const std::filesystem::path& path) const
    -> IModelLoader*
{
  for (const auto& loader : m_Loaders) {
    if (loader->CanLoad(path)) {
      return loader.get();
    }
  }
  return nullptr;
}

auto ModelLoaderRegistry::Load(const std::filesystem::path& path,
                               AssetManager& asset_manager,
                               const ModelLoadOptions& options) const
    -> std::unique_ptr<Model>
{
  auto* loader = GetLoaderFor(path);
  if (loader == nullptr) {
    Logger::Error("No loader registered for: {}", path.string());
    return nullptr;
  }
  return loader->Load(path, asset_manager, options);
}
