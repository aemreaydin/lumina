#ifndef RENDERER_RHI_VULKAN_VULKANSWAPCHAIN_HPP
#define RENDERER_RHI_VULKAN_VULKANSWAPCHAIN_HPP

#include <vector>

#include <SDL3/SDL.h>
#include <volk.h>

#include "Renderer/RHI/RHISwapchain.hpp"

class VulkanDevice;

class VulkanSwapchain final : public RHISwapchain
{
public:
  VulkanSwapchain(const VulkanSwapchain&) = delete;
  VulkanSwapchain(VulkanSwapchain&&) = delete;
  auto operator=(const VulkanSwapchain&) -> VulkanSwapchain& = delete;
  auto operator=(VulkanSwapchain&&) -> VulkanSwapchain& = delete;
  VulkanSwapchain(VulkanDevice& device, SDL_Window* window);
  ~VulkanSwapchain() override;

  void AcquireNextImage(VkSemaphore image_available_semaphore);
  void Resize(uint32_t width, uint32_t height) override;

  auto GetVkSwapchain() const -> VkSwapchainKHR { return m_Swapchain; }

  auto GetImageCount() const -> uint32_t
  {
    return static_cast<uint32_t>(m_Images.size());
  }

  auto GetFormat() const -> VkFormat { return m_Format; }

  [[nodiscard]] auto GetCurrentImage() const -> VkImage
  {
    return m_Images[m_CurrentImageIndex];
  }

  [[nodiscard]] auto GetCurrentImageView() const -> VkImageView
  {
    return m_ImageViews[m_CurrentImageIndex];
  }

  [[nodiscard]] auto GetCurrentImageIndex() const -> uint32_t
  {
    return m_CurrentImageIndex;
  }

  [[nodiscard]] auto GetDepthImage() const -> VkImage { return m_DepthImage; }

  [[nodiscard]] auto GetDepthImageView() const -> VkImageView
  {
    return m_DepthImageView;
  }

  [[nodiscard]] auto GetDepthFormat() const -> VkFormat
  {
    return m_DepthFormat;
  }

private:
  void create_swapchain();
  void cleanup_swapchain();
  void destroy_old_swapchain();
  void create_depth_buffer();
  void cleanup_depth_buffer();

  VulkanDevice& m_Device;
  SDL_Window* m_Window {nullptr};
  VkSurfaceKHR m_Surface {VK_NULL_HANDLE};
  VkSwapchainKHR m_Swapchain {VK_NULL_HANDLE};
  VkSwapchainKHR m_OldSwapchain {VK_NULL_HANDLE};
  VkFormat m_Format {VK_FORMAT_B8G8R8A8_SRGB};
  VkColorSpaceKHR m_ColorSpace {VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};

  std::vector<VkImage> m_Images;
  std::vector<VkImageView> m_ImageViews;
  uint32_t m_CurrentImageIndex {0};

  VkImage m_DepthImage {VK_NULL_HANDLE};
  VkImageView m_DepthImageView {VK_NULL_HANDLE};
  VkDeviceMemory m_DepthImageMemory {VK_NULL_HANDLE};
  VkFormat m_DepthFormat {VK_FORMAT_D32_SFLOAT};
};

#endif
