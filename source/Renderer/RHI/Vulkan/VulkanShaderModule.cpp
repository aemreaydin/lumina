#include <format>
#include <stdexcept>

#include "Renderer/RHI/Vulkan/VulkanShaderModule.hpp"

#include "Core/Logger.hpp"
#include "Renderer/RHI/Vulkan/VulkanDescriptorSet.hpp"
#include "Renderer/RHI/Vulkan/VulkanDevice.hpp"
#include "Renderer/RHI/Vulkan/VulkanUtils.hpp"

// Uses VK_EXT_shader_object for direct shader binding
// TODO: Add fallback to VkShaderModule + VkPipeline for devices without
// extension
VulkanShaderModule::VulkanShaderModule(const VulkanDevice& device,
                                       const ShaderModuleDesc& desc)
    : m_Device(device)
    , m_Stage(desc.Stage)
    , m_EntryPoint(desc.EntryPoint)
{
  if (desc.SPIRVCode.empty()) {
    throw std::runtime_error("Shader SPIR-V code is empty");
  }

  // Extract VkDescriptorSetLayout handles from RHI layouts
  std::vector<VkDescriptorSetLayout> vk_set_layouts;
  vk_set_layouts.reserve(desc.SetLayouts.size());
  for (const auto& layout : desc.SetLayouts) {
    const auto* vk_layout =
        dynamic_cast<const VulkanDescriptorSetLayout*>(layout.get());
    if (vk_layout != nullptr) {
      vk_set_layouts.push_back(vk_layout->GetVkDescriptorSetLayout());
    }
  }

  VkShaderCreateInfoEXT create_info {};
  create_info.sType = VK_STRUCTURE_TYPE_SHADER_CREATE_INFO_EXT;
  create_info.stage = VkUtils::ToVkShaderStage(m_Stage);
  create_info.nextStage = VkUtils::GetNextShaderStage(m_Stage);
  create_info.codeType = VK_SHADER_CODE_TYPE_SPIRV_EXT;
  create_info.codeSize = desc.SPIRVCode.size() * sizeof(uint32_t);
  create_info.pCode = desc.SPIRVCode.data();
  create_info.pName = m_EntryPoint.c_str();
  create_info.setLayoutCount = static_cast<uint32_t>(vk_set_layouts.size());
  create_info.pSetLayouts = vk_set_layouts.data();
  create_info.pushConstantRangeCount = 0;
  create_info.pPushConstantRanges = nullptr;

  if (auto result = VkUtils::Check(vkCreateShadersEXT(
          m_Device.GetVkDevice(), 1, &create_info, nullptr, &m_Shader));
      !result)
  {
    throw std::runtime_error(
        std::format("Failed to create Vulkan shader object: {}",
                    VkUtils::ToString(result.error())));
  }

  Logger::Trace(
      "[Vulkan] Created {} shader object with {} descriptor set layouts",
      ToString(m_Stage),
      vk_set_layouts.size());
}

VulkanShaderModule::~VulkanShaderModule()
{
  if (m_Shader != VK_NULL_HANDLE) {
    vkDestroyShaderEXT(m_Device.GetVkDevice(), m_Shader, nullptr);
  }

  Logger::Trace("[Vulkan] Destroyed shader object");
}
