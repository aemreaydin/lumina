#ifndef RENDERER_RHI_VULKAN_VULKANDESCRIPTORSET_HPP
#define RENDERER_RHI_VULKAN_VULKANDESCRIPTORSET_HPP

#include <memory>

#include <volk.h>

#include "Renderer/RHI/RHIDescriptorSet.hpp"

class VulkanDevice;

class VulkanDescriptorSetLayout : public RHIDescriptorSetLayout
{
public:
  VulkanDescriptorSetLayout(const VulkanDevice& device,
                            const DescriptorSetLayoutDesc& desc);
  ~VulkanDescriptorSetLayout() override;

  VulkanDescriptorSetLayout(const VulkanDescriptorSetLayout&) = delete;
  VulkanDescriptorSetLayout(VulkanDescriptorSetLayout&&) = delete;
  auto operator=(const VulkanDescriptorSetLayout&)
      -> VulkanDescriptorSetLayout& = delete;
  auto operator=(VulkanDescriptorSetLayout&&)
      -> VulkanDescriptorSetLayout& = delete;

  [[nodiscard]] auto GetVkDescriptorSetLayout() const -> VkDescriptorSetLayout
  {
    return m_Layout;
  }

  [[nodiscard]] auto GetBindings() const
      -> const std::vector<DescriptorBinding>&
  {
    return m_Bindings;
  }

private:
  const VulkanDevice& m_Device;
  VkDescriptorSetLayout m_Layout {VK_NULL_HANDLE};
  std::vector<DescriptorBinding> m_Bindings;
};

class VulkanDescriptorSet : public RHIDescriptorSet
{
public:
  VulkanDescriptorSet(const VulkanDevice& device,
                      VkDescriptorPool pool,
                      const std::shared_ptr<RHIDescriptorSetLayout>& layout);
  ~VulkanDescriptorSet() override;

  VulkanDescriptorSet(const VulkanDescriptorSet&) = delete;
  VulkanDescriptorSet(VulkanDescriptorSet&&) = delete;
  auto operator=(const VulkanDescriptorSet&) -> VulkanDescriptorSet& = delete;
  auto operator=(VulkanDescriptorSet&&) -> VulkanDescriptorSet& = delete;

  void WriteBuffer(uint32_t binding,
                   RHIBuffer* buffer,
                   size_t offset,
                   size_t range) override;

  void WriteCombinedImageSampler(uint32_t binding,
                                 RHITexture* texture,
                                 RHISampler* sampler) override;

  [[nodiscard]] auto GetVkDescriptorSet() const -> VkDescriptorSet
  {
    return m_DescriptorSet;
  }

private:
  const VulkanDevice& m_Device;
  VkDescriptorPool m_Pool {VK_NULL_HANDLE};
  VkDescriptorSet m_DescriptorSet {VK_NULL_HANDLE};
  std::shared_ptr<RHIDescriptorSetLayout> m_Layout;
};

#endif
