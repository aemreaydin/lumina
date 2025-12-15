#ifndef RENDERER_RHI_VULKAN_VULKANBUFFER_HPP
#define RENDERER_RHI_VULKAN_VULKANBUFFER_HPP

#include <volk.h>

#include "Renderer/RHI/RHIBuffer.hpp"

class VulkanDevice;

class VulkanBuffer final : public RHIBuffer
{
public:
  VulkanBuffer(const VulkanDevice& device, const BufferDesc& desc);

  VulkanBuffer(const VulkanBuffer&) = delete;
  VulkanBuffer(VulkanBuffer&&) = delete;
  auto operator=(const VulkanBuffer&) -> VulkanBuffer& = delete;
  auto operator=(VulkanBuffer&&) -> VulkanBuffer& = delete;
  ~VulkanBuffer() override;

  [[nodiscard]] auto Map() -> void* override;
  void Unmap() override;
  void Upload(const void* data, size_t size, size_t offset) override;
  [[nodiscard]] auto GetSize() const -> size_t override;

  [[nodiscard]] auto GetVkBuffer() const -> VkBuffer { return m_Buffer; }

private:
  const VulkanDevice& m_Device;
  VkBuffer m_Buffer {VK_NULL_HANDLE};
  VkDeviceMemory m_Memory {VK_NULL_HANDLE};
  size_t m_Size {0};
  bool m_Mapped {false};
  void* m_MappedPtr {nullptr};
};

#endif
