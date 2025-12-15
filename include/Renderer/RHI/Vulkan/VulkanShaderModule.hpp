#ifndef RENDERER_RHI_VULKAN_VULKANSHADERMODULE_HPP
#define RENDERER_RHI_VULKAN_VULKANSHADERMODULE_HPP

#include <string>

#include <volk.h>

#include "Renderer/RHI/RHIShaderModule.hpp"

class VulkanDevice;

// Uses VK_EXT_shader_object for direct shader binding
// TODO: Add fallback to VkShaderModule + VkPipeline for devices without extension
class VulkanShaderModule final : public RHIShaderModule
{
public:
  VulkanShaderModule(const VulkanDevice& device, const ShaderModuleDesc& desc);

  VulkanShaderModule(const VulkanShaderModule&) = delete;
  VulkanShaderModule(VulkanShaderModule&&) = delete;
  auto operator=(const VulkanShaderModule&) -> VulkanShaderModule& = delete;
  auto operator=(VulkanShaderModule&&) -> VulkanShaderModule& = delete;
  ~VulkanShaderModule() override;

  [[nodiscard]] auto GetStage() const -> ShaderStage override { return m_Stage; }
  [[nodiscard]] auto GetVkShaderEXT() const -> VkShaderEXT { return m_Shader; }
  [[nodiscard]] auto GetEntryPoint() const -> const std::string&
  {
    return m_EntryPoint;
  }

private:
  const VulkanDevice& m_Device;
  VkShaderEXT m_Shader {VK_NULL_HANDLE};
  ShaderStage m_Stage {ShaderStage::Vertex};
  std::string m_EntryPoint {"main"};
};

#endif
