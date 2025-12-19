#ifndef RENDERER_RHI_VULKAN_VULKANFRAME_HPP
#define RENDERER_RHI_VULKAN_VULKANFRAME_HPP

#include <volk.h>

#include "Renderer/RHI/Vulkan/VulkanCommandBuffer.hpp"

struct VulkanFrame
{
  VulkanFrame() = default;
  VkSemaphore ImageAvailableSemaphore {VK_NULL_HANDLE};
  VkFence InFlightFence {VK_NULL_HANDLE};
  VkCommandPool CommandPool {VK_NULL_HANDLE};
  VulkanCommandBuffer CommandBuffer;
};

#endif
