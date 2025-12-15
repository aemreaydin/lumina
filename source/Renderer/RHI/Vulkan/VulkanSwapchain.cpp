#include <algorithm>
#include <format>
#include <stdexcept>
#include <vector>

#include "Renderer/RHI/Vulkan/VulkanSwapchain.hpp"

#include "Core/Logger.hpp"
#include "Renderer/RHI/Vulkan/VulkanDevice.hpp"
#include "Renderer/RHI/Vulkan/VulkanUtils.hpp"

VulkanSwapchain::VulkanSwapchain(VulkanDevice& device, SDL_Window* window)
    : m_Device(device)
    , m_Window(window)
    , m_Surface(device.GetVkSurface())
{
  int width = 0;
  int height = 0;
  SDL_GetWindowSizeInPixels(m_Window, &width, &height);
  m_Width = static_cast<uint32_t>(width);
  m_Height = static_cast<uint32_t>(height);

  create_swapchain();
}

VulkanSwapchain::~VulkanSwapchain()
{
  cleanup_swapchain();
  vkDestroySwapchainKHR(m_Device.GetVkDevice(), m_Swapchain, nullptr);
}

void VulkanSwapchain::AcquireNextImage(VkSemaphore image_available_semaphore)
{
  VkDevice device = m_Device.GetVkDevice();

  const auto allowed = {VK_ERROR_OUT_OF_DATE_KHR, VK_SUBOPTIMAL_KHR};
  const auto result =
      VkUtils::Check(vkAcquireNextImageKHR(device,
                                           m_Swapchain,
                                           UINT64_MAX,
                                           image_available_semaphore,
                                           VK_NULL_HANDLE,
                                           &m_CurrentImageIndex),
                     allowed);
  if (result
      && (result.value() == VK_SUBOPTIMAL_KHR
          || result.value() == VK_ERROR_OUT_OF_DATE_KHR))
  {
    Logger::Trace("[Vulkan] Swapchain out of date, resizing");
    Resize(m_Width, m_Height);
    AcquireNextImage(image_available_semaphore);
    return;
  }
  if (!result) {
    throw std::runtime_error(
        std::format("Failed to acquire swapchain image: {}",
                    VkUtils::ToString(result.error())));
  }

  Logger::Trace("[Vulkan] Acquired swapchain image index: {}",
                m_CurrentImageIndex);
}

void VulkanSwapchain::Resize(uint32_t width, uint32_t height)
{
  Logger::Trace("[Vulkan] Resizing swapchain to {}x{}", width, height);
  m_Width = width;
  m_Height = height;

  m_Device.WaitIdle();

  cleanup_swapchain();
  create_swapchain();
  destroy_old_swapchain();
  Logger::Trace("[Vulkan] Swapchain resized successfully");
}

void VulkanSwapchain::create_swapchain()
{
  VkPhysicalDevice physical_device = m_Device.GetVkPhysicalDevice();
  VkDevice device = m_Device.GetVkDevice();

  VkSurfaceCapabilitiesKHR capabilities;
  if (auto result = VkUtils::Check(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
          physical_device, m_Surface, &capabilities));
      !result)
  {
    throw std::runtime_error(
        std::format("Failed to get surface capabilities: {}",
                    VkUtils::ToString(result.error())));
  }

  uint32_t format_count = 0;
  vkGetPhysicalDeviceSurfaceFormatsKHR(
      physical_device, m_Surface, &format_count, nullptr);
  std::vector<VkSurfaceFormatKHR> formats(format_count);
  if (auto result = VkUtils::Check(vkGetPhysicalDeviceSurfaceFormatsKHR(
          physical_device, m_Surface, &format_count, formats.data()));
      !result)
  {
    throw std::runtime_error(std::format("Failed to get surface formats: {}",
                                         VkUtils::ToString(result.error())));
  }

  uint32_t present_mode_count = 0;
  vkGetPhysicalDeviceSurfacePresentModesKHR(
      physical_device, m_Surface, &present_mode_count, nullptr);
  std::vector<VkPresentModeKHR> present_modes(present_mode_count);
  if (auto result = VkUtils::Check(
          vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device,
                                                    m_Surface,
                                                    &present_mode_count,
                                                    present_modes.data()));
      !result)
  {
    throw std::runtime_error(
        std::format("Failed to get surface present modes: {}",
                    VkUtils::ToString(result.error())));
  }

  VkSurfaceFormatKHR surface_format = formats[0];
  for (const auto& format : formats) {
    if (format.format == VK_FORMAT_B8G8R8A8_SRGB
        && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
    {
      surface_format = format;
      break;
    }
  }

  m_Format = surface_format.format;
  m_ColorSpace = surface_format.colorSpace;

  VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
  for (const auto& mode : present_modes) {
    if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
      present_mode = mode;
      break;
    }
  }

  VkExtent2D extent;
  if (capabilities.currentExtent.width != UINT32_MAX) {
    extent = capabilities.currentExtent;
  } else {
    extent = {.width = m_Width, .height = m_Height};
    extent.width = std::clamp(extent.width,
                              capabilities.minImageExtent.width,
                              capabilities.maxImageExtent.width);
    extent.height = std::clamp(extent.height,
                               capabilities.minImageExtent.height,
                               capabilities.maxImageExtent.height);
  }

  m_Width = extent.width;
  m_Height = extent.height;

  uint32_t image_count = std::max(capabilities.minImageCount, 3U);
  if (capabilities.maxImageCount > 0) {
    image_count = std::min(capabilities.maxImageCount, image_count);
  }

  VkSwapchainCreateInfoKHR create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  create_info.surface = m_Surface;
  create_info.minImageCount = image_count;
  create_info.imageFormat = m_Format;
  create_info.imageColorSpace = m_ColorSpace;
  create_info.imageExtent = extent;
  create_info.imageArrayLayers = 1;
  create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  std::array<uint32_t, 2> queue_family_indices {
      m_Device.GetGraphicsQueueFamily(), m_Device.GetPresentQueueFamily()};

  if (m_Device.GetGraphicsQueueFamily() != m_Device.GetPresentQueueFamily()) {
    create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    create_info.queueFamilyIndexCount = queue_family_indices.size();
    create_info.pQueueFamilyIndices = queue_family_indices.data();
  } else {
    create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  }

  create_info.preTransform = capabilities.currentTransform;
  create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  create_info.presentMode = present_mode;
  create_info.clipped = VK_TRUE;
  create_info.oldSwapchain = m_OldSwapchain;

  if (auto result = VkUtils::Check(
          vkCreateSwapchainKHR(device, &create_info, nullptr, &m_Swapchain));
      !result)
  {
    throw std::runtime_error(std::format("Failed to create swapchain: {}",
                                         VkUtils::ToString(result.error())));
  }

  uint32_t image_count_result = 0;
  vkGetSwapchainImagesKHR(device, m_Swapchain, &image_count_result, nullptr);
  m_Images.resize(image_count_result);
  if (auto result = VkUtils::Check(vkGetSwapchainImagesKHR(
          device, m_Swapchain, &image_count_result, m_Images.data()));
      !result)
  {
    throw std::runtime_error(std::format("Failed to get swapchain images: {}",
                                         VkUtils::ToString(result.error())));
  }
  Logger::Trace("Created VkImage with len {}", m_Images.size());

  m_ImageViews.resize(m_Images.size());
  for (size_t i = 0; i < m_Images.size(); ++i) {
    VkImageViewCreateInfo view_info = {};
    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.image = m_Images[i];
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format = m_Format;
    view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view_info.subresourceRange.baseMipLevel = 0;
    view_info.subresourceRange.levelCount = 1;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount = 1;

    if (auto result = VkUtils::Check(
            vkCreateImageView(device, &view_info, nullptr, &m_ImageViews[i]));
        !result)
    {
      throw std::runtime_error(std::format("Failed to create image view: {}",
                                           VkUtils::ToString(result.error())));
    }
  }
  Logger::Trace("Created VkImageView with len {}", m_ImageViews.size());
}

void VulkanSwapchain::cleanup_swapchain()
{
  m_OldSwapchain = m_Swapchain;
  VkDevice device = m_Device.GetVkDevice();

  for (const auto& image_view : m_ImageViews) {
    vkDestroyImageView(device, image_view, nullptr);
  }
  m_ImageViews.clear();
}

void VulkanSwapchain::destroy_old_swapchain()
{
  if (m_OldSwapchain != VK_NULL_HANDLE) {
    vkDestroySwapchainKHR(m_Device.GetVkDevice(), m_OldSwapchain, nullptr);
    m_OldSwapchain = VK_NULL_HANDLE;
  }
}
