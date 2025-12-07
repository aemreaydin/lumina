#ifndef RENDERER_RHI_VULKAN_VULKANFRAME_HPP
#define RENDERER_RHI_VULKAN_VULKANFRAME_HPP

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_handles.hpp>

#include "Renderer/RHI/RHICommandBuffer.hpp"
#include "Renderer/RHI/Vulkan/VulkanBackend.hpp"

struct VulkanFrame
{
  VulkanFrame() = default;
  vk::Semaphore ImageAvailableSemaphore;
  vk::Semaphore RenderFinishedSemaphore;
  vk::Fence InFlightFence;
  vk::CommandPool CommandPool;
  RHICommandBuffer<VulkanBackend> CommandBuffer;
};

#endif
