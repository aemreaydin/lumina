#ifndef RENDERER_RHI_VULKAN_VULKANDEVICE_HPP
#define RENDERER_RHI_VULKAN_VULKANDEVICE_HPP

#include <array>
#include <deque>
#include <memory>

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_handles.hpp>

#include "Renderer/RHI/RHIDevice.hpp"
#include "Renderer/RHI/Vulkan/VulkanCommandBuffer.hpp"
#include "Renderer/RHI/Vulkan/VulkanFrame.hpp"
#include "Renderer/RHI/Vulkan/VulkanSwapchain.hpp"

struct RendererConfig;

class VulkanDevice final : public RHIDevice
{
  static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 3;

  struct PresentInfo
  {
    vk::Semaphore PresentSemaphore;
    vk::Fence PresentFence;
  };

public:
  VulkanDevice() = default;
  VulkanDevice(const VulkanDevice&) = delete;
  VulkanDevice(VulkanDevice&&) = delete;
  auto operator=(const VulkanDevice&) -> VulkanDevice& = delete;
  auto operator=(VulkanDevice&&) -> VulkanDevice& = delete;
  ~VulkanDevice() override = default;

  void Init(const RendererConfig& config, void* window) override;
  void CreateSwapchain(uint32_t width, uint32_t height) override;
  void Destroy() override;

  void BeginFrame() override;
  void EndFrame() override;
  void Present() override;

  [[nodiscard]] auto GetSwapchain() -> RHISwapchain* override;

  [[nodiscard]] auto GetVkInstance() const -> vk::Instance
  {
    return m_Instance;
  }

  [[nodiscard]] auto GetVkSurface() const -> vk::SurfaceKHR
  {
    return m_Surface;
  }

  [[nodiscard]] auto GetVkDevice() const -> vk::Device { return m_Device; }

  [[nodiscard]] auto GetVkPhysicalDevice() const -> vk::PhysicalDevice
  {
    return m_PhysicalDevice;
  }

  [[nodiscard]] auto GetGraphicsQueue() const -> vk::Queue
  {
    return m_GraphicsQueue;
  }

  [[nodiscard]] auto GetPresentQueue() const -> vk::Queue
  {
    return m_PresentQueue;
  }

  [[nodiscard]] auto GetGraphicsQueueFamily() const -> uint32_t
  {
    return m_GraphicsQueueFamily;
  }

  [[nodiscard]] auto GetPresentQueueFamily() const -> uint32_t
  {
    return m_PresentQueueFamily;
  }

private:
  void pick_physical_device(vk::SurfaceKHR surface);
  void create_logical_device(vk::SurfaceKHR surface);
  void setup_debug_messenger();
  [[nodiscard]] auto create_command_pool() -> vk::CommandPool;

  void setup_frame_data();
  [[nodiscard]] auto get_fence() -> vk::Fence;
  void recycle_fence(vk::Fence fence);
  [[nodiscard]] auto get_semaphore() -> vk::Semaphore;
  void recycle_semaphore(vk::Semaphore semaphore);
  void cleanup_present_history();

  vk::Instance m_Instance;
  vk::DebugUtilsMessengerEXT m_DebugMessenger;
  vk::SurfaceKHR m_Surface;
  vk::PhysicalDevice m_PhysicalDevice;
  vk::Device m_Device;
  vk::Queue m_GraphicsQueue;
  vk::Queue m_PresentQueue;
  uint32_t m_GraphicsQueueFamily {0};
  uint32_t m_PresentQueueFamily {0};

  std::unique_ptr<VulkanSwapchain> m_Swapchain;
  GLFWwindow* m_Window {nullptr};
  bool m_Initialized {false};
  bool m_ValidationEnabled {false};

  std::vector<vk::Semaphore> m_SemaphorePool;
  std::vector<vk::Fence> m_FencePool;
  std::deque<PresentInfo> m_PresentInfo;
  std::array<VulkanFrame, MAX_FRAMES_IN_FLIGHT> m_FrameData;
  uint32_t m_CurrentFrameIndex {0};
};

#endif
