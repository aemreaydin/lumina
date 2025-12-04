#ifndef RENDERER_RHI_VULKAN_VULKANSWAPCHAIN_HPP
#define RENDERER_RHI_VULKAN_VULKANSWAPCHAIN_HPP

#include <vector>

#include <vulkan/vulkan.hpp>

#include "Renderer/RHI/RHISwapchain.hpp"

class VulkanDevice;
struct GLFWwindow;

class VulkanSwapchain final : public RHISwapchain
{
public:
  VulkanSwapchain(const VulkanSwapchain&) = delete;
  VulkanSwapchain(VulkanSwapchain&&) = delete;
  auto operator=(const VulkanSwapchain&) -> VulkanSwapchain& = delete;
  auto operator=(VulkanSwapchain&&) -> VulkanSwapchain& = delete;
  VulkanSwapchain(VulkanDevice& device, GLFWwindow* window);
  ~VulkanSwapchain() override;

  auto AcquireNextImage() -> uint32_t override;
  void Present(uint32_t image_index) override;
  void Resize(uint32_t width, uint32_t height) override;

  auto GetVkSwapchain() const -> vk::SwapchainKHR { return m_Swapchain; }

  auto GetImageCount() const -> uint32_t
  {
    return static_cast<uint32_t>(m_Images.size());
  }

  auto GetFormat() const -> vk::Format { return m_Format; }

private:
  void create_swapchain();
  void cleanup_swapchain();

  VulkanDevice& m_Device;
  GLFWwindow* m_Window {nullptr};
  vk::SurfaceKHR m_Surface;
  vk::SwapchainKHR m_Swapchain;
  vk::Format m_Format {vk::Format::eB8G8R8A8Srgb};
  vk::ColorSpaceKHR m_ColorSpace {vk::ColorSpaceKHR::eSrgbNonlinear};

  std::vector<vk::Image> m_Images;
  std::vector<vk::ImageView> m_ImageViews;

  uint32_t m_CurrentImageIndex {0};
  vk::Semaphore m_ImageAvailableSemaphore;
  vk::Semaphore m_RenderFinishedSemaphore;
  vk::Fence m_InFlightFence;
};

#endif
