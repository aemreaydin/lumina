#ifndef RENDERER_MODEL_MATERIAL_HPP
#define RENDERER_MODEL_MATERIAL_HPP

#include <cstdint>
#include <memory>
#include <string>

#include <linalg/vec.hpp>

class RHIBuffer;
class RHIDescriptorSet;
class RHIDescriptorSetLayout;
class RHIDevice;
class RHISampler;
class RHITexture;

struct alignas(16) MaterialProperties
{
  linalg::Vec4 BaseColorFactor {1.0F, 1.0F, 1.0F, 1.0F};
  float MetallicFactor {0.0F};
  float RoughnessFactor {1.0F};
  float AlphaCutoff {0.5F};
  float Padding0 {0.0F};
  linalg::Vec3 EmissiveFactor {0.0F, 0.0F, 0.0F};
  uint32_t Flags {0};
};

// Material flags
enum class MaterialFlags : uint8_t
{
  None = 0,
  HasBaseColorTexture = 1 << 0,
  HasNormalTexture = 1 << 1,
  HasMetallicRoughnessTexture = 1 << 2,
  HasEmissiveTexture = 1 << 3,
  HasOcclusionTexture = 1 << 4,
  DoubleSided = 1 << 5,
};

constexpr auto operator|(MaterialFlags lhs, MaterialFlags rhs) -> MaterialFlags
{
  return static_cast<MaterialFlags>(static_cast<uint32_t>(lhs)
                                    | static_cast<uint32_t>(rhs));
}

constexpr auto operator&(MaterialFlags lhs, MaterialFlags rhs) -> MaterialFlags
{
  return static_cast<MaterialFlags>(static_cast<uint32_t>(lhs)
                                    & static_cast<uint32_t>(rhs));
}

[[nodiscard]] constexpr auto HasFlag(MaterialFlags flags, MaterialFlags flag)
    -> bool
{
  return (static_cast<uint32_t>(flags) & static_cast<uint32_t>(flag)) != 0;
}

enum class AlphaMode : uint8_t
{
  Opaque,
  Mask,
  Blend
};

class Material
{
public:
  Material();
  explicit Material(std::string name);

  Material(const Material&) = delete;
  Material(Material&&) noexcept;
  auto operator=(const Material&) -> Material& = delete;
  auto operator=(Material&&) noexcept -> Material&;
  ~Material();

  // Texture setters (textures are managed externally, e.g., by AssetManager)
  void SetBaseColorTexture(std::shared_ptr<RHITexture> texture);
  void SetNormalTexture(std::shared_ptr<RHITexture> texture);
  void SetMetallicRoughnessTexture(std::shared_ptr<RHITexture> texture);
  void SetEmissiveTexture(std::shared_ptr<RHITexture> texture);
  void SetOcclusionTexture(std::shared_ptr<RHITexture> texture);

  // Property setters
  void SetBaseColorFactor(const linalg::Vec4& color);
  void SetMetallicFactor(float metallic);
  void SetRoughnessFactor(float roughness);
  void SetEmissiveFactor(const linalg::Vec3& emissive);
  void SetAlphaMode(AlphaMode mode);
  void SetAlphaCutoff(float cutoff);
  void SetDoubleSided(bool double_sided);

  // Getters
  [[nodiscard]] auto GetName() const -> const std::string&;
  [[nodiscard]] auto GetProperties() const -> const MaterialProperties&;
  [[nodiscard]] auto GetAlphaMode() const -> AlphaMode;
  [[nodiscard]] auto IsTransparent() const -> bool;
  [[nodiscard]] auto IsDoubleSided() const -> bool;
  [[nodiscard]] auto GetFlags() const -> MaterialFlags;

  // Texture getters
  [[nodiscard]] auto GetBaseColorTexture() const -> RHITexture*;
  [[nodiscard]] auto GetNormalTexture() const -> RHITexture*;
  [[nodiscard]] auto GetMetallicRoughnessTexture() const -> RHITexture*;
  [[nodiscard]] auto GetEmissiveTexture() const -> RHITexture*;
  [[nodiscard]] auto GetOcclusionTexture() const -> RHITexture*;

  // GPU resource management
  void CreateDescriptorSet(
      RHIDevice& device,
      const std::shared_ptr<RHIDescriptorSetLayout>& layout,
      RHISampler* default_sampler,
      RHITexture* default_texture,
      RHITexture* default_normal);

  [[nodiscard]] auto GetDescriptorSet() const -> RHIDescriptorSet*;
  [[nodiscard]] auto GetUniformBuffer() const -> RHIBuffer*;

  // Update uniform buffer with current properties
  void UpdateUniformBuffer();

  // Unique ID for sorting/hashing
  [[nodiscard]] auto GetId() const -> uint64_t;

private:
  void update_flags();

  std::string m_Name {"Unnamed"};
  MaterialProperties m_Properties {};
  MaterialFlags m_Flags {MaterialFlags::None};
  AlphaMode m_AlphaMode {AlphaMode::Opaque};

  // Textures (shared ownership with AssetManager cache)
  std::shared_ptr<RHITexture> m_BaseColorTexture;
  std::shared_ptr<RHITexture> m_NormalTexture;
  std::shared_ptr<RHITexture> m_MetallicRoughnessTexture;
  std::shared_ptr<RHITexture> m_EmissiveTexture;
  std::shared_ptr<RHITexture> m_OcclusionTexture;

  // GPU resources
  std::unique_ptr<RHIBuffer> m_UniformBuffer;
  std::unique_ptr<RHIDescriptorSet> m_DescriptorSet;
  std::shared_ptr<RHIDescriptorSetLayout> m_DescriptorSetLayout;

  uint64_t m_Id {0};
  bool m_Dirty {true};

  static uint64_t SNextId;
};

#endif
