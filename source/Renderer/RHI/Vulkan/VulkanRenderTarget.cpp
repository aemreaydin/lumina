#include "Renderer/RHI/Vulkan/VulkanRenderTarget.hpp"

#include "Core/Logger.hpp"
#include "Renderer/RHI/Vulkan/VulkanTexture.hpp"

VulkanRenderTarget::VulkanRenderTarget(const VulkanDevice& device,
                                       const RenderTargetDesc& desc)
    : m_Width(desc.Width)
    , m_Height(desc.Height)
{
  for (const auto& format : desc.ColorFormats) {
    TextureDesc color_desc;
    color_desc.Width = desc.Width;
    color_desc.Height = desc.Height;
    color_desc.Format = format;
    color_desc.Usage =
        TextureUsage::ColorAttachment | TextureUsage::Sampled;
    m_ColorTextures.push_back(
        std::make_unique<VulkanTexture>(device, color_desc));
  }

  if (desc.HasDepth) {
    TextureDesc depth_desc;
    depth_desc.Width = desc.Width;
    depth_desc.Height = desc.Height;
    depth_desc.Format = desc.DepthFormat;
    depth_desc.Usage = TextureUsage::DepthStencilAttachment;
    m_DepthTexture = std::make_unique<VulkanTexture>(device, depth_desc);
  }

  Logger::Trace("[Vulkan] Created render target {}x{} with {} color attachment(s)",
                desc.Width,
                desc.Height,
                m_ColorTextures.size());
}

auto VulkanRenderTarget::GetWidth() const -> uint32_t
{
  return m_Width;
}

auto VulkanRenderTarget::GetHeight() const -> uint32_t
{
  return m_Height;
}

auto VulkanRenderTarget::GetColorTexture(size_t index) -> RHITexture*
{
  if (index >= m_ColorTextures.size()) {
    return nullptr;
  }
  return m_ColorTextures[index].get();
}

auto VulkanRenderTarget::GetColorTextureCount() const -> size_t
{
  return m_ColorTextures.size();
}

auto VulkanRenderTarget::GetDepthTexture() -> RHITexture*
{
  return m_DepthTexture.get();
}

auto VulkanRenderTarget::GetColorImage(size_t index) const -> VkImage
{
  if (index >= m_ColorTextures.size()) {
    return VK_NULL_HANDLE;
  }
  return m_ColorTextures[index] ? m_ColorTextures[index]->GetVkImage()
                                : VK_NULL_HANDLE;
}

auto VulkanRenderTarget::GetColorImageView(size_t index) const -> VkImageView
{
  if (index >= m_ColorTextures.size()) {
    return VK_NULL_HANDLE;
  }
  return m_ColorTextures[index] ? m_ColorTextures[index]->GetVkImageView()
                                : VK_NULL_HANDLE;
}

auto VulkanRenderTarget::GetDepthImage() const -> VkImage
{
  return m_DepthTexture ? m_DepthTexture->GetVkImage() : VK_NULL_HANDLE;
}

auto VulkanRenderTarget::GetDepthImageView() const -> VkImageView
{
  return m_DepthTexture ? m_DepthTexture->GetVkImageView() : VK_NULL_HANDLE;
}
