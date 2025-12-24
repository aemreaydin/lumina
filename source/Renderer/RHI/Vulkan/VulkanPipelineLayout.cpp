#include <ranges>
#include <stdexcept>

#include "Renderer/RHI/Vulkan/VulkanPipelineLayout.hpp"

#include <vulkan/vulkan_core.h>

#include "Core/Logger.hpp"
#include "Renderer/RHI/Vulkan/VulkanDescriptorSet.hpp"
#include "Renderer/RHI/Vulkan/VulkanDevice.hpp"
#include "Renderer/RHI/Vulkan/VulkanUtils.hpp"

VulkanPipelineLayout::VulkanPipelineLayout(const VulkanDevice& device,
                                           const PipelineLayoutDesc& desc)
    : m_Device(device)
    , m_SetLayouts(desc.SetLayouts)
{
  std::vector<VkDescriptorSetLayout> vk_set_layouts(desc.SetLayouts.size());
  for (const auto& [index, layout] :
       std::ranges::views::enumerate(desc.SetLayouts))
  {
    const auto* vk_layout =
        dynamic_cast<const VulkanDescriptorSetLayout*>(layout.get());
    if (vk_layout == nullptr) {
      throw std::runtime_error(
          "Invalid descriptor set layout for Vulkan pipeline layout");
    }
    vk_set_layouts.at(static_cast<size_t>(index)) =
        (vk_layout->GetVkDescriptorSetLayout());
  }

  std::vector<VkPushConstantRange> vk_push_constants(desc.PushConstants.size());
  for (const auto& [index, push_constant] :
       std::ranges::views::enumerate(desc.PushConstants))
  {
    vk_push_constants.at(static_cast<size_t>(index)) = {
        .stageFlags = VkUtils::ToVkShaderStageFlags(push_constant.Stages),
        .offset = push_constant.Offset,
        .size = push_constant.Size};
  }

  VkPipelineLayoutCreateInfo layout_info {};
  layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  layout_info.setLayoutCount = static_cast<uint32_t>(vk_set_layouts.size());
  layout_info.pSetLayouts = vk_set_layouts.data();
  layout_info.pushConstantRangeCount =
      static_cast<uint32_t>(vk_push_constants.size());
  layout_info.pPushConstantRanges = vk_push_constants.data();

  if (auto result = VkUtils::Check(vkCreatePipelineLayout(
          m_Device.GetVkDevice(), &layout_info, nullptr, &m_PipelineLayout));
      !result)
  {
    throw std::runtime_error("Failed to create Vulkan pipeline layout");
  }

  Logger::Trace("[Vulkan] Created pipeline layout with {} set layouts",
                desc.SetLayouts.size());
}

VulkanPipelineLayout::~VulkanPipelineLayout()
{
  if (m_PipelineLayout != VK_NULL_HANDLE) {
    vkDestroyPipelineLayout(m_Device.GetVkDevice(), m_PipelineLayout, nullptr);
    Logger::Trace("[Vulkan] Destroyed pipeline layout");
  }
}
