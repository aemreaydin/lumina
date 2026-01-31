#ifndef RENDERER_RHI_VULKAN_VULKANIMGUI_HPP
#define RENDERER_RHI_VULKAN_VULKANIMGUI_HPP

#include <volk.h>

#include "UI/RHIImGui.hpp"

class VulkanDevice;

/**
 * @brief Vulkan backend implementation for ImGui rendering.
 *
 * Uses ImGui's Vulkan backend with dynamic rendering (no render pass objects).
 * Creates its own descriptor pool for ImGui's internal use.
 */
class VulkanImGui final : public RHIImGui
{
public:
  explicit VulkanImGui(VulkanDevice& device);
  ~VulkanImGui() override = default;

  VulkanImGui(const VulkanImGui&) = delete;
  VulkanImGui(VulkanImGui&&) = delete;
  auto operator=(const VulkanImGui&) -> VulkanImGui& = delete;
  auto operator=(VulkanImGui&&) -> VulkanImGui& = delete;

  void Init(Window& window) override;
  void Shutdown() override;
  void BeginFrame() override;
  void EndFrame() override;

private:
  VulkanDevice& m_Device;
  VkDescriptorPool m_DescriptorPool {VK_NULL_HANDLE};
  VkFormat m_ColorFormat {VK_FORMAT_UNDEFINED};
};

#endif
