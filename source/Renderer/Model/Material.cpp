#include "Renderer/Model/Material.hpp"

#include "Renderer/RHI/RHIBuffer.hpp"
#include "Renderer/RHI/RHIDescriptorSet.hpp"
#include "Renderer/RHI/RHIDevice.hpp"
#include "Renderer/RHI/RHISampler.hpp"
#include "Renderer/RHI/RHITexture.hpp"

uint64_t Material::SNextId = 1;

Material::Material()
    : m_Id(SNextId++)
{
}

Material::Material(std::string name)
    : m_Name(std::move(name))
    , m_Id(SNextId++)
{
}

Material::Material(Material&&) noexcept = default;
auto Material::operator=(Material&&) noexcept -> Material& = default;
Material::~Material() = default;

void Material::SetBaseColorTexture(std::shared_ptr<RHITexture> texture)
{
  m_BaseColorTexture = std::move(texture);
  update_flags();
  m_Dirty = true;
}

void Material::SetNormalTexture(std::shared_ptr<RHITexture> texture)
{
  m_NormalTexture = std::move(texture);
  update_flags();
  m_Dirty = true;
}

void Material::SetMetallicRoughnessTexture(std::shared_ptr<RHITexture> texture)
{
  m_MetallicRoughnessTexture = std::move(texture);
  update_flags();
  m_Dirty = true;
}

void Material::SetEmissiveTexture(std::shared_ptr<RHITexture> texture)
{
  m_EmissiveTexture = std::move(texture);
  update_flags();
  m_Dirty = true;
}

void Material::SetOcclusionTexture(std::shared_ptr<RHITexture> texture)
{
  m_OcclusionTexture = std::move(texture);
  update_flags();
  m_Dirty = true;
}

void Material::SetBaseColorFactor(const glm::vec4& color)
{
  m_Properties.BaseColorFactor = color;
  m_Dirty = true;
}

void Material::SetMetallicFactor(float metallic)
{
  m_Properties.MetallicFactor = metallic;
  m_Dirty = true;
}

void Material::SetRoughnessFactor(float roughness)
{
  m_Properties.RoughnessFactor = roughness;
  m_Dirty = true;
}

void Material::SetEmissiveFactor(const glm::vec3& emissive)
{
  m_Properties.EmissiveFactor = emissive;
  m_Dirty = true;
}

void Material::SetAlphaMode(AlphaMode mode)
{
  m_AlphaMode = mode;
}

void Material::SetAlphaCutoff(float cutoff)
{
  m_Properties.AlphaCutoff = cutoff;
  m_Dirty = true;
}

void Material::SetDoubleSided(bool double_sided)
{
  if (double_sided) {
    m_Flags = m_Flags | MaterialFlags::DoubleSided;
  } else {
    m_Flags = static_cast<MaterialFlags>(
        static_cast<uint32_t>(m_Flags)
        & ~static_cast<uint32_t>(MaterialFlags::DoubleSided));
  }
  update_flags();
  m_Dirty = true;
}

auto Material::GetName() const -> const std::string&
{
  return m_Name;
}

auto Material::GetProperties() const -> const MaterialProperties&
{
  return m_Properties;
}

auto Material::GetAlphaMode() const -> AlphaMode
{
  return m_AlphaMode;
}

auto Material::IsTransparent() const -> bool
{
  return m_AlphaMode == AlphaMode::Blend;
}

auto Material::IsDoubleSided() const -> bool
{
  return HasFlag(m_Flags, MaterialFlags::DoubleSided);
}

auto Material::GetFlags() const -> MaterialFlags
{
  return m_Flags;
}

auto Material::GetBaseColorTexture() const -> RHITexture*
{
  return m_BaseColorTexture.get();
}

auto Material::GetNormalTexture() const -> RHITexture*
{
  return m_NormalTexture.get();
}

auto Material::GetMetallicRoughnessTexture() const -> RHITexture*
{
  return m_MetallicRoughnessTexture.get();
}

auto Material::GetEmissiveTexture() const -> RHITexture*
{
  return m_EmissiveTexture.get();
}

auto Material::GetOcclusionTexture() const -> RHITexture*
{
  return m_OcclusionTexture.get();
}

void Material::CreateDescriptorSet(
    RHIDevice& device,
    const std::shared_ptr<RHIDescriptorSetLayout>& layout,
    RHISampler* default_sampler,
    RHITexture* default_texture,
    RHITexture* default_normal)
{
  m_DescriptorSetLayout = layout;

  // Create uniform buffer for material properties
  BufferDesc buffer_desc {};
  buffer_desc.Size = sizeof(MaterialProperties);
  buffer_desc.Usage = BufferUsage::Uniform;
  buffer_desc.CPUVisible = true;
  m_UniformBuffer = device.CreateBuffer(buffer_desc);

  // Create descriptor set
  m_DescriptorSet = device.CreateDescriptorSet(layout);

  // Write uniform buffer (binding 0)
  m_DescriptorSet->WriteBuffer(
      0, m_UniformBuffer.get(), 0, sizeof(MaterialProperties));

  // Write base color texture (binding 1)
  RHITexture* base_color =
      m_BaseColorTexture ? m_BaseColorTexture.get() : default_texture;
  m_DescriptorSet->WriteCombinedImageSampler(1, base_color, default_sampler);

  // Write normal texture (binding 2)
  RHITexture* normal = m_NormalTexture ? m_NormalTexture.get() : default_normal;
  m_DescriptorSet->WriteCombinedImageSampler(2, normal, default_sampler);

  // Upload initial properties
  UpdateUniformBuffer();
}

auto Material::GetDescriptorSet() const -> RHIDescriptorSet*
{
  return m_DescriptorSet.get();
}

auto Material::GetUniformBuffer() const -> RHIBuffer*
{
  return m_UniformBuffer.get();
}

void Material::UpdateUniformBuffer()
{
  if (m_UniformBuffer && m_Dirty) {
    m_Properties.Flags = static_cast<uint32_t>(m_Flags);
    m_UniformBuffer->Upload(&m_Properties, sizeof(MaterialProperties), 0);
    m_Dirty = false;
  }
}

auto Material::GetId() const -> uint64_t
{
  return m_Id;
}

void Material::update_flags()
{
  auto non_texture_flags = static_cast<uint32_t>(m_Flags)
      & static_cast<uint32_t>(MaterialFlags::DoubleSided);
  m_Flags = static_cast<MaterialFlags>(non_texture_flags);

  if (m_BaseColorTexture) {
    m_Flags = m_Flags | MaterialFlags::HasBaseColorTexture;
  }
  if (m_NormalTexture) {
    m_Flags = m_Flags | MaterialFlags::HasNormalTexture;
  }
  if (m_MetallicRoughnessTexture) {
    m_Flags = m_Flags | MaterialFlags::HasMetallicRoughnessTexture;
  }
  if (m_EmissiveTexture) {
    m_Flags = m_Flags | MaterialFlags::HasEmissiveTexture;
  }
  if (m_OcclusionTexture) {
    m_Flags = m_Flags | MaterialFlags::HasOcclusionTexture;
  }

  m_Properties.Flags = static_cast<uint32_t>(m_Flags);
}
