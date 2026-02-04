#ifndef RENDERER_RHI_VULKAN_VULKANRENDERTARGET_HPP
#define RENDERER_RHI_VULKAN_VULKANRENDERTARGET_HPP

#include <memory>

#include <volk.h>

#include "Renderer/RHI/RHIRenderTarget.hpp"

class VulkanDevice;
class VulkanTexture;

class VulkanRenderTarget final : public RHIRenderTarget
{
public:
  VulkanRenderTarget(const VulkanDevice& device, const RenderTargetDesc& desc);

  VulkanRenderTarget(const VulkanRenderTarget&) = delete;
  VulkanRenderTarget(VulkanRenderTarget&&) = delete;
  auto operator=(const VulkanRenderTarget&) -> VulkanRenderTarget& = delete;
  auto operator=(VulkanRenderTarget&&) -> VulkanRenderTarget& = delete;
  ~VulkanRenderTarget() override = default;

  [[nodiscard]] auto GetWidth() const -> uint32_t override;
  [[nodiscard]] auto GetHeight() const -> uint32_t override;
  [[nodiscard]] auto GetColorTexture() -> RHITexture* override;
  [[nodiscard]] auto GetDepthTexture() -> RHITexture* override;

  [[nodiscard]] auto GetColorImage() const -> VkImage;
  [[nodiscard]] auto GetColorImageView() const -> VkImageView;
  [[nodiscard]] auto GetDepthImage() const -> VkImage;
  [[nodiscard]] auto GetDepthImageView() const -> VkImageView;

private:
  uint32_t m_Width {0};
  uint32_t m_Height {0};
  std::unique_ptr<VulkanTexture> m_ColorTexture;
  std::unique_ptr<VulkanTexture> m_DepthTexture;
};

#endif
