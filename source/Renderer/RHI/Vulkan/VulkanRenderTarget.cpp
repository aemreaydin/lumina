#include "Renderer/RHI/Vulkan/VulkanRenderTarget.hpp"

#include "Core/Logger.hpp"
#include "Renderer/RHI/Vulkan/VulkanTexture.hpp"

VulkanRenderTarget::VulkanRenderTarget(const VulkanDevice& device,
                                       const RenderTargetDesc& desc)
    : m_Width(desc.Width)
    , m_Height(desc.Height)
{
  TextureDesc color_desc;
  color_desc.Width = desc.Width;
  color_desc.Height = desc.Height;
  color_desc.Format = desc.ColorFormat;
  color_desc.Usage =
      TextureUsage::ColorAttachment | TextureUsage::Sampled;
  m_ColorTexture = std::make_unique<VulkanTexture>(device, color_desc);

  if (desc.HasDepth) {
    TextureDesc depth_desc;
    depth_desc.Width = desc.Width;
    depth_desc.Height = desc.Height;
    depth_desc.Format = desc.DepthFormat;
    depth_desc.Usage = TextureUsage::DepthStencilAttachment;
    m_DepthTexture = std::make_unique<VulkanTexture>(device, depth_desc);
  }

  Logger::Trace(
      "[Vulkan] Created render target {}x{}", desc.Width, desc.Height);
}

auto VulkanRenderTarget::GetWidth() const -> uint32_t
{
  return m_Width;
}

auto VulkanRenderTarget::GetHeight() const -> uint32_t
{
  return m_Height;
}

auto VulkanRenderTarget::GetColorTexture() -> RHITexture*
{
  return m_ColorTexture.get();
}

auto VulkanRenderTarget::GetDepthTexture() -> RHITexture*
{
  return m_DepthTexture.get();
}

auto VulkanRenderTarget::GetColorImage() const -> VkImage
{
  return m_ColorTexture ? m_ColorTexture->GetVkImage() : VK_NULL_HANDLE;
}

auto VulkanRenderTarget::GetColorImageView() const -> VkImageView
{
  return m_ColorTexture ? m_ColorTexture->GetVkImageView() : VK_NULL_HANDLE;
}

auto VulkanRenderTarget::GetDepthImage() const -> VkImage
{
  return m_DepthTexture ? m_DepthTexture->GetVkImage() : VK_NULL_HANDLE;
}

auto VulkanRenderTarget::GetDepthImageView() const -> VkImageView
{
  return m_DepthTexture ? m_DepthTexture->GetVkImageView() : VK_NULL_HANDLE;
}
