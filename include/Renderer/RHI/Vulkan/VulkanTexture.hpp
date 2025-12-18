#ifndef RENDERER_RHI_VULKAN_VULKANTEXTURE_HPP
#define RENDERER_RHI_VULKAN_VULKANTEXTURE_HPP

#include <volk.h>

#include "Renderer/RHI/RHITexture.hpp"

class VulkanDevice;

class VulkanTexture final : public RHITexture
{
public:
  VulkanTexture(const VulkanDevice& device, const TextureDesc& desc);

  VulkanTexture(const VulkanTexture&) = delete;
  VulkanTexture(VulkanTexture&&) = delete;
  auto operator=(const VulkanTexture&) -> VulkanTexture& = delete;
  auto operator=(VulkanTexture&&) -> VulkanTexture& = delete;
  ~VulkanTexture() override;

  [[nodiscard]] auto GetWidth() const -> uint32_t override;
  [[nodiscard]] auto GetHeight() const -> uint32_t override;
  [[nodiscard]] auto GetFormat() const -> TextureFormat override;
  void Upload(const void* data, size_t size) override;

  [[nodiscard]] auto GetVkImage() const -> VkImage { return m_Image; }

  [[nodiscard]] auto GetVkImageView() const -> VkImageView
  {
    return m_ImageView;
  }

private:
  const VulkanDevice& m_Device;
  VkImage m_Image {VK_NULL_HANDLE};
  VkImageView m_ImageView {VK_NULL_HANDLE};
  VkDeviceMemory m_Memory {VK_NULL_HANDLE};
  uint32_t m_Width {0};
  uint32_t m_Height {0};
  TextureFormat m_Format {TextureFormat::RGBA8Unorm};
  VkFormat m_VkFormat {VK_FORMAT_R8G8B8A8_UNORM};
};

#endif
