#ifndef RENDERER_ASSET_ASSETMANAGER_HPP
#define RENDERER_ASSET_ASSETMANAGER_HPP

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>

class RHIDevice;
class RHITexture;
class RHISampler;
class RHIDescriptorSetLayout;
class Model;
class Material;

struct TextureLoadOptions
{
  bool GenerateMipmaps {false};
  bool SRGB {true};
  bool FlipY {false};
};

struct ModelLoadOptions
{
  bool CalculateNormals {true};
  bool CalculateTangents {true};
  bool FlipUVs {true};
  float Scale {1.0F};
};

class AssetManager
{
public:
  explicit AssetManager(RHIDevice& device);
  ~AssetManager();
  AssetManager(const AssetManager&) = delete;
  AssetManager(AssetManager&&) = delete;
  auto operator=(const AssetManager&) -> AssetManager& = delete;
  auto operator=(AssetManager&&) -> AssetManager& = delete;

  [[nodiscard]] auto LoadTexture(const std::filesystem::path& path,
                                 const TextureLoadOptions& options = {})
      -> std::shared_ptr<RHITexture>;

  [[nodiscard]] auto GetTexture(const std::filesystem::path& path) const
      -> std::shared_ptr<RHITexture>;

  [[nodiscard]] auto HasTexture(const std::filesystem::path& path) const
      -> bool;

  [[nodiscard]] auto LoadModel(const std::filesystem::path& path,
                               const ModelLoadOptions& options = {})
      -> std::shared_ptr<Model>;

  [[nodiscard]] auto GetModel(const std::filesystem::path& path) const
      -> std::shared_ptr<Model>;

  [[nodiscard]] auto HasModel(const std::filesystem::path& path) const -> bool;

  [[nodiscard]] auto GetDefaultTexture() const -> RHITexture*;
  [[nodiscard]] auto GetDefaultNormalMap() const -> RHITexture*;
  [[nodiscard]] auto GetDefaultSampler() const -> RHISampler*;
  [[nodiscard]] auto GetMaterialDescriptorSetLayout() const
      -> std::shared_ptr<RHIDescriptorSetLayout>;
  void SetMaterialDescriptorSetLayout(
      std::shared_ptr<RHIDescriptorSetLayout> layout);

  void UnloadUnusedAssets();
  void UnloadAll();

  [[nodiscard]] auto GetLoadedTextureCount() const -> size_t;
  [[nodiscard]] auto GetLoadedModelCount() const -> size_t;

  void SetAssetBasePath(const std::filesystem::path& path);
  [[nodiscard]] auto GetAssetBasePath() const -> const std::filesystem::path&;

  [[nodiscard]] auto GetDevice() -> RHIDevice&;

private:
  void create_default_resources();
  [[nodiscard]] auto resolve_asset_path(const std::filesystem::path& path) const
      -> std::filesystem::path;
  [[nodiscard]] static auto GetCanonicalKey(const std::filesystem::path& path)
      -> std::string;

  RHIDevice& m_Device;
  std::filesystem::path m_AssetBasePath {"assets"};

  std::unordered_map<std::string, std::shared_ptr<RHITexture>> m_TextureCache;
  std::unordered_map<std::string, std::shared_ptr<Model>> m_ModelCache;

  std::unique_ptr<RHITexture> m_DefaultTexture;
  std::unique_ptr<RHITexture> m_DefaultNormalMap;
  std::unique_ptr<RHISampler> m_DefaultSampler;
  std::shared_ptr<RHIDescriptorSetLayout> m_MaterialDescriptorSetLayout;
};

#endif
