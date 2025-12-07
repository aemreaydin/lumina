#include <algorithm>
#include <stdexcept>
#include <vector>

#include "Renderer/RHI/Vulkan/VulkanSwapchain.hpp"

#include <GLFW/glfw3.h>

#include "Core/Logger.hpp"
#include "Renderer/RHI/Vulkan/VulkanDevice.hpp"

VulkanSwapchain::VulkanSwapchain(VulkanDevice& device, GLFWwindow* window)
    : m_Device(device)
    , m_Window(window)
{
  // Get surface from device
  m_Surface = device.GetVkSurface();

  // Get window size
  int width = 0;
  int height = 0;
  glfwGetFramebufferSize(m_Window, &width, &height);
  m_Width = static_cast<uint32_t>(width);
  m_Height = static_cast<uint32_t>(height);

  // Create swapchain
  create_swapchain();
}

VulkanSwapchain::~VulkanSwapchain()
{
  cleanup_swapchain();
}

void VulkanSwapchain::AcquireNextImage(vk::Semaphore image_available_semaphore)
{
  auto device = m_Device.GetVkDevice();

  auto result = device.acquireNextImageKHR(
      m_Swapchain, UINT64_MAX, image_available_semaphore, nullptr);

  if (result.result == vk::Result::eErrorOutOfDateKHR) {
    Logger::Trace("[Vulkan] Swapchain out of date, resizing");
    Resize(m_Width, m_Height);
    AcquireNextImage(image_available_semaphore);
    return;
  }

  if (result.result != vk::Result::eSuccess
      && result.result != vk::Result::eSuboptimalKHR)
  {
    throw std::runtime_error("Failed to acquire swapchain image");
  }

  m_CurrentImageIndex = result.value;
  Logger::Trace("[Vulkan] Acquired swapchain image index: {}",
                m_CurrentImageIndex);
}

void VulkanSwapchain::Resize(uint32_t width, uint32_t height)
{
  Logger::Trace("[Vulkan] Resizing swapchain to {}x{}", width, height);
  m_Width = width;
  m_Height = height;

  m_Device.GetVkDevice().waitIdle();

  cleanup_swapchain();
  create_swapchain();
  Logger::Trace("[Vulkan] Swapchain resized successfully");
}

void VulkanSwapchain::create_swapchain()
{
  const vk::SurfaceCapabilitiesKHR capabilities =
      m_Device.GetVkPhysicalDevice().getSurfaceCapabilitiesKHR(m_Surface);

  const std::vector<vk::SurfaceFormatKHR> formats =
      m_Device.GetVkPhysicalDevice().getSurfaceFormatsKHR(m_Surface);

  const std::vector<vk::PresentModeKHR> present_modes =
      m_Device.GetVkPhysicalDevice().getSurfacePresentModesKHR(m_Surface);

  vk::SurfaceFormatKHR surface_format = formats[0];
  for (const auto& format : formats) {
    if (format.format == vk::Format::eB8G8R8A8Srgb
        && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
    {
      surface_format = format;
      break;
    }
  }

  m_Format = surface_format.format;
  m_ColorSpace = surface_format.colorSpace;

  vk::PresentModeKHR present_mode = vk::PresentModeKHR::eFifo;
  for (const auto& mode : present_modes) {
    if (mode == vk::PresentModeKHR::eMailbox) {
      present_mode = mode;
      break;
    }
  }

  vk::Extent2D extent;
  if (capabilities.currentExtent.width != UINT32_MAX) {
    extent = capabilities.currentExtent;
  } else {
    extent = vk::Extent2D {m_Width, m_Height};
    extent.width = std::clamp(extent.width,
                              capabilities.minImageExtent.width,
                              capabilities.maxImageExtent.width);
    extent.height = std::clamp(extent.height,
                               capabilities.minImageExtent.height,
                               capabilities.maxImageExtent.height);
  }

  m_Width = extent.width;
  m_Height = extent.height;

  uint32_t image_count = capabilities.minImageCount + 1;
  if (capabilities.maxImageCount > 0
      && image_count > capabilities.maxImageCount)
  {
    image_count = capabilities.maxImageCount;
  }

  vk::SwapchainCreateInfoKHR create_info {};
  create_info.surface = m_Surface;
  create_info.minImageCount = image_count;
  create_info.imageFormat = m_Format;
  create_info.imageColorSpace = m_ColorSpace;
  create_info.imageExtent = extent;
  create_info.imageArrayLayers = 1;
  create_info.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;

  std::array<uint32_t, 2> queue_family_indices {
      m_Device.GetGraphicsQueueFamily(), m_Device.GetPresentQueueFamily()};

  if (m_Device.GetGraphicsQueueFamily() != m_Device.GetPresentQueueFamily()) {
    create_info.imageSharingMode = vk::SharingMode::eConcurrent;
    create_info.queueFamilyIndexCount = queue_family_indices.size();
    create_info.pQueueFamilyIndices = queue_family_indices.data();
  } else {
    create_info.imageSharingMode = vk::SharingMode::eExclusive;
  }

  create_info.preTransform = capabilities.currentTransform;
  create_info.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
  create_info.presentMode = present_mode;
  create_info.clipped = VK_TRUE;

  m_Swapchain = m_Device.GetVkDevice().createSwapchainKHR(create_info);

  m_Images = m_Device.GetVkDevice().getSwapchainImagesKHR(m_Swapchain);
  Logger::Trace("Created vk::Image with len {}", m_Images.size());

  m_ImageViews.resize(m_Images.size());
  for (size_t i = 0; i < m_Images.size(); ++i) {
    vk::ImageViewCreateInfo view_info {};
    view_info.image = m_Images[i];
    view_info.viewType = vk::ImageViewType::e2D;
    view_info.format = m_Format;
    view_info.components.r = vk::ComponentSwizzle::eIdentity;
    view_info.components.g = vk::ComponentSwizzle::eIdentity;
    view_info.components.b = vk::ComponentSwizzle::eIdentity;
    view_info.components.a = vk::ComponentSwizzle::eIdentity;
    view_info.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    view_info.subresourceRange.baseMipLevel = 0;
    view_info.subresourceRange.levelCount = 1;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount = 1;

    m_ImageViews[i] = m_Device.GetVkDevice().createImageView(view_info);
  }
  Logger::Trace("Created vk::ImageView with len {}", m_ImageViews.size());
}

void VulkanSwapchain::cleanup_swapchain()
{
  auto device = m_Device.GetVkDevice();

  for (auto image_view : m_ImageViews) {
    device.destroyImageView(image_view);
  }
  m_ImageViews.clear();

  if (m_Swapchain) {
    device.destroySwapchainKHR(m_Swapchain);
  }
}
