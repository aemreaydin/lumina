#ifndef RENDERER_RHI_VULKAN_VULKANSAMPLER_HPP
#define RENDERER_RHI_VULKAN_VULKANSAMPLER_HPP

#include <volk.h>

#include "Renderer/RHI/RHISampler.hpp"

class VulkanDevice;

class VulkanSampler final : public RHISampler
{
public:
  VulkanSampler(const VulkanDevice& device, const SamplerDesc& desc);

  VulkanSampler(const VulkanSampler&) = delete;
  VulkanSampler(VulkanSampler&&) = delete;
  auto operator=(const VulkanSampler&) -> VulkanSampler& = delete;
  auto operator=(VulkanSampler&&) -> VulkanSampler& = delete;
  ~VulkanSampler() override;

  [[nodiscard]] auto GetVkSampler() const -> VkSampler { return m_Sampler; }

private:
  const VulkanDevice& m_Device;
  VkSampler m_Sampler {VK_NULL_HANDLE};
};

#endif
