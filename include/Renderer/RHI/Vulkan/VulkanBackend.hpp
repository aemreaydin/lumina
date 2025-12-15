#ifndef RENDERER_RHI_VULKAN_VULKANBACKEND_HPP
#define RENDERER_RHI_VULKAN_VULKANBACKEND_HPP

#include <volk.h>

// Vulkan backend type traits
struct VulkanBackend
{
  using CommandBufferHandle = VkCommandBuffer;
  using PipelineHandle = VkPipeline;
  using BufferHandle = VkBuffer;
};

#endif
