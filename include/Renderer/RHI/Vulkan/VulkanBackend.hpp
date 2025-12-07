#ifndef RENDERER_RHI_VULKAN_VULKANBACKEND_HPP
#define RENDERER_RHI_VULKAN_VULKANBACKEND_HPP

#include <vulkan/vulkan.hpp>

// Vulkan backend type traits
struct VulkanBackend
{
  using CommandBufferHandle = vk::CommandBuffer;
  using PipelineHandle = vk::Pipeline;
  using BufferHandle = vk::Buffer;
};

#endif
