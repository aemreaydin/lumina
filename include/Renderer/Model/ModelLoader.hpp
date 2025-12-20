#ifndef RENDERER_MODEL_MODELLOADER_HPP
#define RENDERER_MODEL_MODELLOADER_HPP

#include <filesystem>
#include <memory>
#include <vector>

class Model;
class AssetManager;
struct ModelLoadOptions;

// Abstract base for model format loaders
class IModelLoader
{
public:
  IModelLoader() = default;
  IModelLoader(const IModelLoader&) = default;
  IModelLoader(IModelLoader&&) = default;
  auto operator=(const IModelLoader&) -> IModelLoader& = default;
  auto operator=(IModelLoader&&) -> IModelLoader& = default;
  virtual ~IModelLoader() = default;

  [[nodiscard]] virtual auto CanLoad(const std::filesystem::path& path) const
      -> bool = 0;

  [[nodiscard]] virtual auto Load(const std::filesystem::path& path,
                                  AssetManager& asset_manager,
                                  const ModelLoadOptions& options)
      -> std::unique_ptr<Model> = 0;
};

// OBJ format loader
class OBJModelLoader : public IModelLoader
{
public:
  [[nodiscard]] auto CanLoad(const std::filesystem::path& path) const
      -> bool override;

  [[nodiscard]] auto Load(const std::filesystem::path& path,
                          AssetManager& asset_manager,
                          const ModelLoadOptions& options)
      -> std::unique_ptr<Model> override;
};

// Registry of loaders - selects appropriate loader by file extension
class ModelLoaderRegistry
{
public:
  static auto Instance() -> ModelLoaderRegistry&;

  void RegisterLoader(std::unique_ptr<IModelLoader> loader);

  [[nodiscard]] auto GetLoaderFor(const std::filesystem::path& path) const
      -> IModelLoader*;

  [[nodiscard]] auto Load(const std::filesystem::path& path,
                          AssetManager& asset_manager,
                          const ModelLoadOptions& options) const
      -> std::unique_ptr<Model>;

private:
  ModelLoaderRegistry();

  std::vector<std::unique_ptr<IModelLoader>> m_Loaders;
};

#endif
