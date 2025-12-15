#include <cstring>
#include <format>
#include <stdexcept>

#include "Renderer/RHI/Vulkan/VulkanBuffer.hpp"

#include "Core/Logger.hpp"
#include "Renderer/RHI/Vulkan/VulkanDevice.hpp"
#include "Renderer/RHI/Vulkan/VulkanUtils.hpp"

VulkanBuffer::VulkanBuffer(const VulkanDevice& device, const BufferDesc& desc)
    : m_Device(device)
    , m_Size(desc.Size)
{
  VkDevice vk_device = m_Device.GetVkDevice();

  // Convert BufferUsage to VkBufferUsageFlags
  VkBufferUsageFlags usage_flags = 0;
  if ((desc.Usage & BufferUsage::Vertex) != BufferUsage {}) {
    usage_flags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
  }
  if ((desc.Usage & BufferUsage::Index) != BufferUsage {}) {
    usage_flags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
  }
  if ((desc.Usage & BufferUsage::Uniform) != BufferUsage {}) {
    usage_flags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
  }
  if ((desc.Usage & BufferUsage::TransferSrc) != BufferUsage {}) {
    usage_flags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  }
  if ((desc.Usage & BufferUsage::TransferDst) != BufferUsage {}) {
    usage_flags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  }

  // Create buffer
  VkBufferCreateInfo buffer_info = {};
  buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  buffer_info.size = desc.Size;
  buffer_info.usage = usage_flags;
  buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (auto result = VkUtils::Check(
          vkCreateBuffer(vk_device, &buffer_info, nullptr, &m_Buffer));
      !result)
  {
    throw std::runtime_error(std::format("Failed to create Vulkan buffer: {}",
                                         VkUtils::ToString(result.error())));
  }

  // Get memory requirements
  VkMemoryRequirements mem_requirements;
  vkGetBufferMemoryRequirements(vk_device, m_Buffer, &mem_requirements);

  // Determine memory properties
  VkMemoryPropertyFlags properties = 0;
  if (desc.CPUVisible) {
    properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
        | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
  } else {
    properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
  }

  // Find suitable memory type
  VkPhysicalDeviceMemoryProperties mem_properties;
  vkGetPhysicalDeviceMemoryProperties(m_Device.GetVkPhysicalDevice(),
                                      &mem_properties);

  uint32_t memory_type_index = UINT32_MAX;
  for (uint32_t i = 0; i < mem_properties.memoryTypeCount; ++i) {
    if ((mem_requirements.memoryTypeBits & (1 << i)) != 0
        && (mem_properties.memoryTypes[i].propertyFlags & properties)
            == properties)
    {
      memory_type_index = i;
      break;
    }
  }

  if (memory_type_index == UINT32_MAX) {
    throw std::runtime_error("Failed to find suitable memory type for buffer");
  }

  // Allocate memory
  VkMemoryAllocateInfo alloc_info = {};
  alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc_info.allocationSize = mem_requirements.size;
  alloc_info.memoryTypeIndex = memory_type_index;

  if (auto result = VkUtils::Check(
          vkAllocateMemory(vk_device, &alloc_info, nullptr, &m_Memory));
      !result)
  {
    vkDestroyBuffer(vk_device, m_Buffer, nullptr);
    throw std::runtime_error(std::format("Failed to allocate buffer memory: {}",
                                         VkUtils::ToString(result.error())));
  }

  // Bind buffer to memory
  if (auto result =
          VkUtils::Check(vkBindBufferMemory(vk_device, m_Buffer, m_Memory, 0));
      !result)
  {
    vkFreeMemory(vk_device, m_Memory, nullptr);
    vkDestroyBuffer(vk_device, m_Buffer, nullptr);
    throw std::runtime_error(std::format("Failed to bind buffer memory: {}",
                                         VkUtils::ToString(result.error())));
  }

  Logger::Trace("[Vulkan] Created buffer with size {}", desc.Size);
}

VulkanBuffer::~VulkanBuffer()
{
  VkDevice vk_device = m_Device.GetVkDevice();

  if (m_Mapped) {
    Unmap();
  }

  if (m_Memory != VK_NULL_HANDLE) {
    vkFreeMemory(vk_device, m_Memory, nullptr);
  }

  if (m_Buffer != VK_NULL_HANDLE) {
    vkDestroyBuffer(vk_device, m_Buffer, nullptr);
  }

  Logger::Trace("[Vulkan] Destroyed buffer");
}

auto VulkanBuffer::Map() -> void*
{
  if (m_Mapped) {
    return m_MappedPtr;
  }

  VkDevice vk_device = m_Device.GetVkDevice();

  if (auto result = VkUtils::Check(
          vkMapMemory(vk_device, m_Memory, 0, m_Size, 0, &m_MappedPtr));
      !result)
  {
    throw std::runtime_error(std::format("Failed to map buffer memory: {}",
                                         VkUtils::ToString(result.error())));
  }

  m_Mapped = true;
  return m_MappedPtr;
}

void VulkanBuffer::Upload(const void* data, size_t size, size_t offset)
{
  void* mapped = Map();
  const std::span dest =
      std::span {static_cast<std::byte*>(mapped), m_Size}.subspan(offset, size);
  std::memcpy(dest.data(), data, size);
  Unmap();
}

void VulkanBuffer::Unmap()
{
  if (!m_Mapped) {
    return;
  }

  VkDevice vk_device = m_Device.GetVkDevice();
  vkUnmapMemory(vk_device, m_Memory);
  m_MappedPtr = nullptr;
  m_Mapped = false;
}

auto VulkanBuffer::GetSize() const -> size_t
{
  return m_Size;
}
