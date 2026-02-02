#ifndef RENDERER_RHI_VULKAN_VULKANPIPELINELAYOUT_HPP
#define RENDERER_RHI_VULKAN_VULKANPIPELINELAYOUT_HPP

#include <memory>
#include <vector>

#include <volk.h>

#include "Renderer/RHI/RHIPipeline.hpp"

class RHIDescriptorSetLayout;
class VulkanDevice;

class VulkanPipelineLayout : public RHIPipelineLayout
{
public:
  VulkanPipelineLayout(
      const VulkanDevice& device,
      const std::vector<std::shared_ptr<RHIDescriptorSetLayout>>& set_layouts);
  ~VulkanPipelineLayout() override;

  VulkanPipelineLayout(const VulkanPipelineLayout&) = delete;
  VulkanPipelineLayout(VulkanPipelineLayout&&) = delete;
  auto operator=(const VulkanPipelineLayout&) -> VulkanPipelineLayout& = delete;
  auto operator=(VulkanPipelineLayout&&) -> VulkanPipelineLayout& = delete;

  [[nodiscard]] auto GetVkPipelineLayout() const -> VkPipelineLayout
  {
    return m_PipelineLayout;
  }

  [[nodiscard]] auto GetSetLayouts() const
      -> const std::vector<std::shared_ptr<RHIDescriptorSetLayout>>&
  {
    return m_SetLayouts;
  }

private:
  const VulkanDevice& m_Device;
  VkPipelineLayout m_PipelineLayout {VK_NULL_HANDLE};
  std::vector<std::shared_ptr<RHIDescriptorSetLayout>> m_SetLayouts;
};

#endif
