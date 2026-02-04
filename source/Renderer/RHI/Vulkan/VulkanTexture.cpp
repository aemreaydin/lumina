#include <cstring>
#include <format>
#include <stdexcept>

#include "Renderer/RHI/Vulkan/VulkanTexture.hpp"

#include <vulkan/vulkan_core.h>

#include "Core/Logger.hpp"
#include "Renderer/RHI/Vulkan/VulkanDevice.hpp"
#include "Renderer/RHI/Vulkan/VulkanUtils.hpp"

static auto ToVkFormat(TextureFormat format) -> VkFormat
{
  switch (format) {
    case TextureFormat::R8Unorm:
      return VK_FORMAT_R8_UNORM;
    case TextureFormat::RG8Unorm:
      return VK_FORMAT_R8G8_UNORM;
    case TextureFormat::RGB8Unorm:
      return VK_FORMAT_R8G8B8_UNORM;
    case TextureFormat::RGB8Srgb:
      return VK_FORMAT_R8G8B8_SRGB;
    case TextureFormat::RGBA8Unorm:
      return VK_FORMAT_R8G8B8A8_UNORM;
    case TextureFormat::RGBA8Srgb:
      return VK_FORMAT_R8G8B8A8_SRGB;
    case TextureFormat::BGRA8Unorm:
      return VK_FORMAT_B8G8R8A8_UNORM;
    case TextureFormat::RGBA16F:
      return VK_FORMAT_R16G16B16A16_SFLOAT;
    case TextureFormat::RGBA32F:
      return VK_FORMAT_R32G32B32A32_SFLOAT;
    case TextureFormat::Depth24Stencil8:
      return VK_FORMAT_D24_UNORM_S8_UINT;
    case TextureFormat::Depth32F:
      return VK_FORMAT_D32_SFLOAT;
  }
  return VK_FORMAT_R8G8B8A8_UNORM;
}

static auto IsDepthFormat(TextureFormat format) -> bool
{
  return format == TextureFormat::Depth32F
      || format == TextureFormat::Depth24Stencil8;
}

static auto ToVkImageUsageFlags(TextureUsage usage) -> VkImageUsageFlags
{
  VkImageUsageFlags flags = 0;
  if ((usage & TextureUsage::Sampled) == TextureUsage::Sampled) {
    flags |= VK_IMAGE_USAGE_SAMPLED_BIT;
  }
  if ((usage & TextureUsage::Storage) == TextureUsage::Storage) {
    flags |= VK_IMAGE_USAGE_STORAGE_BIT;
  }
  if ((usage & TextureUsage::TransferDst) == TextureUsage::TransferDst) {
    flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  }
  if ((usage & TextureUsage::TransferSrc) == TextureUsage::TransferSrc) {
    flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
  }
  if ((usage & TextureUsage::ColorAttachment)
      == TextureUsage::ColorAttachment)
  {
    flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  }
  if ((usage & TextureUsage::DepthStencilAttachment)
      == TextureUsage::DepthStencilAttachment)
  {
    flags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
  }
  // Backward compat: plain Sampled textures also get TRANSFER_DST for Upload()
  if (flags == VK_IMAGE_USAGE_SAMPLED_BIT) {
    flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  }
  return flags;
}

static auto FindMemoryType(VkPhysicalDevice physical_device,
                           uint32_t type_filter,
                           VkMemoryPropertyFlags properties) -> uint32_t
{
  VkPhysicalDeviceMemoryProperties mem_properties;
  vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_properties);

  for (uint32_t i = 0; i < mem_properties.memoryTypeCount; ++i) {
    if ((type_filter & (1 << i)) != 0
        && (mem_properties.memoryTypes[i].propertyFlags & properties)
            == properties)
    {
      return i;
    }
  }

  throw std::runtime_error("Failed to find suitable memory type");
}

VulkanTexture::VulkanTexture(const VulkanDevice& device,
                             const TextureDesc& desc)
    : m_Device(device)
    , m_Width(desc.Width)
    , m_Height(desc.Height)
    , m_Format(desc.Format)
    , m_VkFormat(ToVkFormat(desc.Format))
{
  VkDevice vk_device = m_Device.GetVkDevice();

  // Create image
  VkImageCreateInfo image_info = {};
  image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  image_info.imageType = VK_IMAGE_TYPE_2D;
  image_info.format = m_VkFormat;
  image_info.extent = {.width = desc.Width, .height = desc.Height, .depth = 1};
  image_info.mipLevels = desc.MipLevels;
  image_info.arrayLayers = 1;
  image_info.samples = VK_SAMPLE_COUNT_1_BIT;
  image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
  image_info.usage = ToVkImageUsageFlags(desc.Usage);
  image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

  if (auto result = VkUtils::Check(
          vkCreateImage(vk_device, &image_info, nullptr, &m_Image));
      !result)
  {
    throw std::runtime_error(std::format("Failed to create Vulkan image: {}",
                                         VkUtils::ToString(result.error())));
  }

  // Get memory requirements
  VkMemoryRequirements mem_requirements;
  vkGetImageMemoryRequirements(vk_device, m_Image, &mem_requirements);

  // Allocate memory
  VkMemoryAllocateInfo alloc_info = {};
  alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc_info.allocationSize = mem_requirements.size;
  alloc_info.memoryTypeIndex =
      FindMemoryType(m_Device.GetVkPhysicalDevice(),
                     mem_requirements.memoryTypeBits,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  if (auto result = VkUtils::Check(
          vkAllocateMemory(vk_device, &alloc_info, nullptr, &m_Memory));
      !result)
  {
    vkDestroyImage(vk_device, m_Image, nullptr);
    throw std::runtime_error(std::format("Failed to allocate image memory: {}",
                                         VkUtils::ToString(result.error())));
  }

  // Bind image to memory
  if (auto result =
          VkUtils::Check(vkBindImageMemory(vk_device, m_Image, m_Memory, 0));
      !result)
  {
    vkFreeMemory(vk_device, m_Memory, nullptr);
    vkDestroyImage(vk_device, m_Image, nullptr);
    throw std::runtime_error(std::format("Failed to bind image memory: {}",
                                         VkUtils::ToString(result.error())));
  }

  // Create image view
  VkImageViewCreateInfo view_info = {};
  view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  view_info.image = m_Image;
  view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
  view_info.format = m_VkFormat;
  view_info.subresourceRange.aspectMask = IsDepthFormat(desc.Format)
      ? VK_IMAGE_ASPECT_DEPTH_BIT
      : VK_IMAGE_ASPECT_COLOR_BIT;
  view_info.subresourceRange.baseMipLevel = 0;
  view_info.subresourceRange.levelCount = desc.MipLevels;
  view_info.subresourceRange.baseArrayLayer = 0;
  view_info.subresourceRange.layerCount = 1;

  if (auto result = VkUtils::Check(
          vkCreateImageView(vk_device, &view_info, nullptr, &m_ImageView));
      !result)
  {
    vkFreeMemory(vk_device, m_Memory, nullptr);
    vkDestroyImage(vk_device, m_Image, nullptr);
    throw std::runtime_error(std::format("Failed to create image view: {}",
                                         VkUtils::ToString(result.error())));
  }

  Logger::Trace("[Vulkan] Created texture {}x{}", desc.Width, desc.Height);
}

VulkanTexture::~VulkanTexture()
{
  VkDevice vk_device = m_Device.GetVkDevice();

  if (m_ImageView != VK_NULL_HANDLE) {
    vkDestroyImageView(vk_device, m_ImageView, nullptr);
  }
  if (m_Memory != VK_NULL_HANDLE) {
    vkFreeMemory(vk_device, m_Memory, nullptr);
  }
  if (m_Image != VK_NULL_HANDLE) {
    vkDestroyImage(vk_device, m_Image, nullptr);
  }

  Logger::Trace("[Vulkan] Destroyed texture");
}

auto VulkanTexture::GetWidth() const -> uint32_t
{
  return m_Width;
}

auto VulkanTexture::GetHeight() const -> uint32_t
{
  return m_Height;
}

auto VulkanTexture::GetFormat() const -> TextureFormat
{
  return m_Format;
}

void VulkanTexture::Upload(const void* data, size_t size)
{
  VkDevice vk_device = m_Device.GetVkDevice();
  VkQueue queue = m_Device.GetGraphicsQueue();

  // Create staging buffer
  VkBufferCreateInfo buffer_info = {};
  buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  buffer_info.size = size;
  buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VkBuffer staging_buffer = nullptr;
  vkCreateBuffer(vk_device, &buffer_info, nullptr, &staging_buffer);

  VkMemoryRequirements mem_requirements;
  vkGetBufferMemoryRequirements(vk_device, staging_buffer, &mem_requirements);

  VkMemoryAllocateInfo alloc_info = {};
  alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc_info.allocationSize = mem_requirements.size;
  alloc_info.memoryTypeIndex =
      FindMemoryType(m_Device.GetVkPhysicalDevice(),
                     mem_requirements.memoryTypeBits,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                         | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  VkDeviceMemory staging_memory = nullptr;
  vkAllocateMemory(vk_device, &alloc_info, nullptr, &staging_memory);
  vkBindBufferMemory(vk_device, staging_buffer, staging_memory, 0);

  // Copy data to staging buffer
  void* mapped = nullptr;
  vkMapMemory(vk_device, staging_memory, 0, size, 0, &mapped);
  std::memcpy(mapped, data, size);
  vkUnmapMemory(vk_device, staging_memory);

  // Create command pool and buffer for all upload operations
  VkCommandPoolCreateInfo pool_info = {};
  pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  pool_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
  pool_info.queueFamilyIndex = m_Device.GetGraphicsQueueFamily();

  VkCommandPool command_pool = nullptr;
  vkCreateCommandPool(vk_device, &pool_info, nullptr, &command_pool);

  VkCommandBufferAllocateInfo cmd_alloc_info = {};
  cmd_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  cmd_alloc_info.commandPool = command_pool;
  cmd_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  cmd_alloc_info.commandBufferCount = 1;

  VkCommandBuffer cmd_buffer = nullptr;
  vkAllocateCommandBuffers(vk_device, &cmd_alloc_info, &cmd_buffer);

  VkCommandBufferBeginInfo begin_info = {};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  vkBeginCommandBuffer(cmd_buffer, &begin_info);

  // Barrier 1: UNDEFINED -> TRANSFER_DST_OPTIMAL
  VkImageMemoryBarrier barrier = {};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = m_Image;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;
  barrier.srcAccessMask = 0;
  barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

  vkCmdPipelineBarrier(cmd_buffer,
                       VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                       VK_PIPELINE_STAGE_TRANSFER_BIT,
                       0,
                       0,
                       nullptr,
                       0,
                       nullptr,
                       1,
                       &barrier);

  // Copy buffer to image
  VkBufferImageCopy region = {};
  region.bufferOffset = 0;
  region.bufferRowLength = 0;
  region.bufferImageHeight = 0;
  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.mipLevel = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount = 1;
  region.imageOffset = {.x = 0, .y = 0, .z = 0};
  region.imageExtent = {.width = m_Width, .height = m_Height, .depth = 1};

  vkCmdCopyBufferToImage(cmd_buffer,
                         staging_buffer,
                         m_Image,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                         1,
                         &region);

  // Barrier 2: TRANSFER_DST_OPTIMAL -> SHADER_READ_ONLY_OPTIMAL
  barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

  vkCmdPipelineBarrier(cmd_buffer,
                       VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                       0,
                       0,
                       nullptr,
                       0,
                       nullptr,
                       1,
                       &barrier);

  vkEndCommandBuffer(cmd_buffer);

  // Submit and wait
  VkSubmitInfo submit_info = {};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &cmd_buffer;

  vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE);
  vkQueueWaitIdle(queue);

  // Cleanup
  vkFreeCommandBuffers(vk_device, command_pool, 1, &cmd_buffer);
  vkDestroyCommandPool(vk_device, command_pool, nullptr);
  vkDestroyBuffer(vk_device, staging_buffer, nullptr);
  vkFreeMemory(vk_device, staging_memory, nullptr);

  Logger::Trace("[Vulkan] Uploaded texture data");
}
