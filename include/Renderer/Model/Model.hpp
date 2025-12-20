#ifndef RENDERER_MODEL_MODEL_HPP
#define RENDERER_MODEL_MODEL_HPP

#include <memory>
#include <string>
#include <vector>

#include "Renderer/Model/BoundingVolume.hpp"

class Mesh;
class Material;
class RHIDevice;
class RHIDescriptorSetLayout;
class RHISampler;
class RHITexture;

// Model: A complete asset containing meshes and materials
class Model
{
public:
  Model() = default;
  explicit Model(std::string name);

  Model(const Model&) = delete;
  Model(Model&&) noexcept;
  auto operator=(const Model&) -> Model& = delete;
  auto operator=(Model&&) noexcept -> Model&;
  ~Model();

  // Add meshes and materials (called by ModelLoader)
  void AddMesh(std::unique_ptr<Mesh> mesh);
  void AddMaterial(std::unique_ptr<Material> material);

  // Create all GPU resources
  void CreateResources(
      RHIDevice& device,
      const std::shared_ptr<RHIDescriptorSetLayout>& material_layout,
      RHISampler* default_sampler,
      RHITexture* default_texture,
      RHITexture* default_normal);

  void DestroyResources();

  // Getters
  [[nodiscard]] auto GetName() const -> const std::string&;

  [[nodiscard]] auto GetMeshes() const
      -> const std::vector<std::unique_ptr<Mesh>>&;
  [[nodiscard]] auto GetMesh(size_t index) const -> Mesh*;
  [[nodiscard]] auto GetMeshCount() const -> size_t;

  [[nodiscard]] auto GetMaterials() const
      -> const std::vector<std::unique_ptr<Material>>&;
  [[nodiscard]] auto GetMaterial(size_t index) const -> Material*;
  [[nodiscard]] auto GetMaterialCount() const -> size_t;

  [[nodiscard]] auto GetBounds() const -> const AABB&;
  [[nodiscard]] auto AreResourcesCreated() const -> bool;

  // Source path (for reloading/debugging)
  void SetSourcePath(const std::string& path);
  [[nodiscard]] auto GetSourcePath() const -> const std::string&;

private:
  void compute_bounds();

  std::string m_Name {"Unnamed"};
  std::string m_SourcePath;

  std::vector<std::unique_ptr<Mesh>> m_Meshes;
  std::vector<std::unique_ptr<Material>> m_Materials;

  AABB m_Bounds {};
  bool m_ResourcesCreated {false};
};

#endif
