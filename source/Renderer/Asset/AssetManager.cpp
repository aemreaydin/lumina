#include <array>

#include "Renderer/Asset/AssetManager.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "Core/Logger.hpp"
#include "Renderer/Model/Model.hpp"
#include "Renderer/Model/ModelLoader.hpp"
#include "Renderer/RHI/RHIDescriptorSet.hpp"
#include "Renderer/RHI/RHIDevice.hpp"
#include "Renderer/RHI/RHISampler.hpp"
#include "Renderer/RHI/RHITexture.hpp"

AssetManager::AssetManager(RHIDevice& device)
    : m_Device(device)
{
  create_default_resources();
}

AssetManager::~AssetManager()
{
  UnloadAll();
}

auto AssetManager::LoadTexture(const std::filesystem::path& path,
                               const TextureLoadOptions& options)
    -> std::shared_ptr<RHITexture>
{
  auto resolved_path = resolve_asset_path(path);
  auto key = GetCanonicalKey(resolved_path);

  auto iter = m_TextureCache.find(key);
  if (iter != m_TextureCache.end()) {
    return iter->second;
  }

  int width = 0;
  int height = 0;
  int channels = 0;

  stbi_set_flip_vertically_on_load(options.FlipY ? 1 : 0);
  auto* data = stbi_load(resolved_path.string().c_str(),
                         &width,
                         &height,
                         &channels,
                         STBI_rgb_alpha);

  if (data == nullptr) {
    Logger::Error("Failed to load texture: {}", resolved_path.string());
    return nullptr;
  }

  TextureDesc desc {};
  desc.Width = static_cast<uint32_t>(width);
  desc.Height = static_cast<uint32_t>(height);
  desc.Format =
      options.SRGB ? TextureFormat::RGBA8Srgb : TextureFormat::RGBA8Unorm;
  desc.Usage = TextureUsage::Sampled;
  desc.MipLevels = 1;

  auto texture = m_Device.CreateTexture(desc);
  texture->Upload(data, static_cast<size_t>(width * height) * 4);

  stbi_image_free(data);

  auto shared_texture = std::shared_ptr<RHITexture>(std::move(texture));
  m_TextureCache[key] = shared_texture;

  Logger::Info(
      "Loaded texture: {} ({}x{})", resolved_path.string(), width, height);
  return shared_texture;
}

auto AssetManager::GetTexture(const std::filesystem::path& path) const
    -> std::shared_ptr<RHITexture>
{
  auto resolved_path = resolve_asset_path(path);
  auto key = GetCanonicalKey(resolved_path);
  auto iter = m_TextureCache.find(key);
  if (iter != m_TextureCache.end()) {
    return iter->second;
  }
  return nullptr;
}

auto AssetManager::HasTexture(const std::filesystem::path& path) const -> bool
{
  auto resolved_path = resolve_asset_path(path);
  auto key = GetCanonicalKey(resolved_path);
  return m_TextureCache.contains(key);
}

auto AssetManager::LoadModel(const std::filesystem::path& path,
                             const ModelLoadOptions& options)
    -> std::shared_ptr<Model>
{
  auto resolved_path = resolve_asset_path(path);
  auto key = GetCanonicalKey(resolved_path);

  auto iter = m_ModelCache.find(key);
  if (iter != m_ModelCache.end()) {
    return iter->second;
  }

  auto model =
      ModelLoaderRegistry::Instance().Load(resolved_path, *this, options);
  if (!model) {
    Logger::Error("Failed to load model: {}", resolved_path.string());
    return nullptr;
  }

  model->SetSourcePath(resolved_path.string());

  model->CreateResources(m_Device,
                         m_MaterialDescriptorSetLayout,
                         m_DefaultSampler.get(),
                         m_DefaultTexture.get(),
                         m_DefaultNormalMap.get());

  auto shared_model = std::shared_ptr<Model>(std::move(model));
  m_ModelCache[key] = shared_model;

  Logger::Info("Loaded model: {} ({} meshes, {} materials)",
               resolved_path.string(),
               shared_model->GetMeshCount(),
               shared_model->GetMaterialCount());

  return shared_model;
}

auto AssetManager::GetModel(const std::filesystem::path& path) const
    -> std::shared_ptr<Model>
{
  auto resolved_path = resolve_asset_path(path);
  auto key = GetCanonicalKey(resolved_path);
  auto iter = m_ModelCache.find(key);
  if (iter != m_ModelCache.end()) {
    return iter->second;
  }
  return nullptr;
}

auto AssetManager::HasModel(const std::filesystem::path& path) const -> bool
{
  auto resolved_path = resolve_asset_path(path);
  auto key = GetCanonicalKey(resolved_path);
  return m_ModelCache.contains(key);
}

auto AssetManager::GetDefaultTexture() const -> RHITexture*
{
  return m_DefaultTexture.get();
}

auto AssetManager::GetDefaultNormalMap() const -> RHITexture*
{
  return m_DefaultNormalMap.get();
}

auto AssetManager::GetDefaultSampler() const -> RHISampler*
{
  return m_DefaultSampler.get();
}

auto AssetManager::GetMaterialDescriptorSetLayout() const
    -> std::shared_ptr<RHIDescriptorSetLayout>
{
  return m_MaterialDescriptorSetLayout;
}

void AssetManager::SetMaterialDescriptorSetLayout(
    std::shared_ptr<RHIDescriptorSetLayout> layout)
{
  m_MaterialDescriptorSetLayout = std::move(layout);
}

void AssetManager::UnloadUnusedAssets()
{
  for (auto iter = m_TextureCache.begin(); iter != m_TextureCache.end();) {
    if (iter->second.use_count() == 1) {
      iter = m_TextureCache.erase(iter);
    } else {
      ++iter;
    }
  }

  for (auto iter = m_ModelCache.begin(); iter != m_ModelCache.end();) {
    if (iter->second.use_count() == 1) {
      iter = m_ModelCache.erase(iter);
    } else {
      ++iter;
    }
  }
}

void AssetManager::UnloadAll()
{
  m_TextureCache.clear();
  m_ModelCache.clear();
}

auto AssetManager::GetLoadedTextureCount() const -> size_t
{
  return m_TextureCache.size();
}

auto AssetManager::GetLoadedModelCount() const -> size_t
{
  return m_ModelCache.size();
}

void AssetManager::SetAssetBasePath(const std::filesystem::path& path)
{
  m_AssetBasePath = path;
}

auto AssetManager::GetAssetBasePath() const -> const std::filesystem::path&
{
  return m_AssetBasePath;
}

auto AssetManager::GetDevice() -> RHIDevice&
{
  return m_Device;
}

void AssetManager::create_default_resources()
{
  {
    TextureDesc desc {};
    desc.Width = 1;
    desc.Height = 1;
    desc.Format = TextureFormat::RGBA8Unorm;
    desc.Usage = TextureUsage::Sampled;
    desc.MipLevels = 1;

    m_DefaultTexture = m_Device.CreateTexture(desc);
    std::array<uint8_t, 4> white = {255, 255, 255, 255};
    m_DefaultTexture->Upload(white.data(), white.size());
  }

  {
    TextureDesc desc {};
    desc.Width = 1;
    desc.Height = 1;
    desc.Format = TextureFormat::RGBA8Unorm;
    desc.Usage = TextureUsage::Sampled;
    desc.MipLevels = 1;

    m_DefaultNormalMap = m_Device.CreateTexture(desc);
    std::array<uint8_t, 4> normal = {128, 128, 255, 255};
    m_DefaultNormalMap->Upload(normal.data(), normal.size());
  }

  {
    SamplerDesc desc {};
    desc.MinFilter = Filter::Linear;
    desc.MagFilter = Filter::Linear;
    desc.MipFilter = Filter::Linear;
    desc.AddressModeU = SamplerAddressMode::Repeat;
    desc.AddressModeV = SamplerAddressMode::Repeat;

    desc.EnableAnisotropy = false;
    desc.MaxAnisotropy = 16.0F;

    m_DefaultSampler = m_Device.CreateSampler(desc);
  }
}

auto AssetManager::resolve_asset_path(const std::filesystem::path& path) const
    -> std::filesystem::path
{
  if (path.is_absolute()) {
    return path;
  }
  return m_AssetBasePath / path;
}

auto AssetManager::GetCanonicalKey(const std::filesystem::path& path)
    -> std::string
{
  std::error_code error_code;
  auto canonical = std::filesystem::canonical(path, error_code);
  if (error_code) {
    return path.string();
  }
  return canonical.string();
}
