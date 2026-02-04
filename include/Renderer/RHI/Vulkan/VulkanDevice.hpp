#ifndef RENDERER_RHI_VULKAN_VULKANDEVICE_HPP
#define RENDERER_RHI_VULKAN_VULKANDEVICE_HPP

#include <array>
#include <memory>
#include <vector>

#include <SDL3/SDL.h>
#include <volk.h>

#include "Renderer/RHI/RHIDevice.hpp"
#include "Renderer/RHI/RenderPassInfo.hpp"
#include "Renderer/RHI/Vulkan/VulkanCommandBuffer.hpp"
#include "Renderer/RHI/Vulkan/VulkanFrame.hpp"
#include "Renderer/RHI/Vulkan/VulkanSwapchain.hpp"

struct RendererConfig;

class VulkanDevice final : public RHIDevice
{
  static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 3;

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
  void WaitIdle() override;

  [[nodiscard]] auto GetSwapchain() const -> RHISwapchain* override;
  [[nodiscard]] auto GetCurrentCommandBuffer() -> RHICommandBuffer* override;

  [[nodiscard]] auto CreateRenderTarget(const RenderTargetDesc& desc)
      -> std::unique_ptr<RHIRenderTarget> override;
  [[nodiscard]] auto CreateBuffer(const BufferDesc& desc)
      -> std::unique_ptr<RHIBuffer> override;
  [[nodiscard]] auto CreateTexture(const TextureDesc& desc)
      -> std::unique_ptr<RHITexture> override;
  [[nodiscard]] auto CreateSampler(const SamplerDesc& desc)
      -> std::unique_ptr<RHISampler> override;
  [[nodiscard]] auto CreateShaderModule(const ShaderModuleDesc& desc)
      -> std::unique_ptr<RHIShaderModule> override;
  [[nodiscard]] auto CreateGraphicsPipeline(const GraphicsPipelineDesc& desc)
      -> std::unique_ptr<RHIGraphicsPipeline> override;
  [[nodiscard]] auto CreateDescriptorSetLayout(
      const DescriptorSetLayoutDesc& desc)
      -> std::shared_ptr<RHIDescriptorSetLayout> override;
  [[nodiscard]] auto CreateDescriptorSet(
      const std::shared_ptr<RHIDescriptorSetLayout>& layout)
      -> std::unique_ptr<RHIDescriptorSet> override;
  [[nodiscard]] auto CreatePipelineLayout(
      const std::vector<std::shared_ptr<RHIDescriptorSetLayout>>& set_layouts)
      -> std::shared_ptr<RHIPipelineLayout> override;

  [[nodiscard]] auto GetVkInstance() const -> VkInstance { return m_Instance; }

  [[nodiscard]] auto GetVkSurface() const -> VkSurfaceKHR { return m_Surface; }

  [[nodiscard]] auto GetVkDevice() const -> VkDevice { return m_Device; }

  [[nodiscard]] auto GetVkPhysicalDevice() const -> VkPhysicalDevice
  {
    return m_PhysicalDevice;
  }

  [[nodiscard]] auto GetGraphicsQueue() const -> VkQueue
  {
    return m_GraphicsQueue;
  }

  [[nodiscard]] auto GetPresentQueue() const -> VkQueue
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

  [[nodiscard]] static constexpr auto GetApiVersion() -> uint32_t
  {
    return VK_API_VERSION_1_3;
  }

  [[nodiscard]] auto GetSwapchainFormat() const -> VkFormat
  {
    return m_Swapchain ? m_Swapchain->GetFormat() : VK_FORMAT_B8G8R8A8_SRGB;
  }

  [[nodiscard]] auto GetDepthFormat() const -> VkFormat
  {
    return m_Swapchain ? m_Swapchain->GetDepthFormat() : VK_FORMAT_D32_SFLOAT;
  }

  [[nodiscard]] auto IsDepthEnabled() const -> bool { return m_DepthEnabled; }

private:
  void pick_physical_device(VkSurfaceKHR surface);
  void create_logical_device(VkSurfaceKHR surface);
  void setup_debug_messenger();
  [[nodiscard]] auto create_command_pool() -> VkCommandPool;
  void create_descriptor_pool();

  void setup_frame_data();

  VkInstance m_Instance {VK_NULL_HANDLE};
  VkDebugUtilsMessengerEXT m_DebugMessenger {VK_NULL_HANDLE};
  VkSurfaceKHR m_Surface {VK_NULL_HANDLE};
  VkPhysicalDevice m_PhysicalDevice {VK_NULL_HANDLE};
  VkDevice m_Device {VK_NULL_HANDLE};
  VkQueue m_GraphicsQueue {VK_NULL_HANDLE};
  VkQueue m_PresentQueue {VK_NULL_HANDLE};
  uint32_t m_GraphicsQueueFamily {0};
  uint32_t m_PresentQueueFamily {0};
  VkDescriptorPool m_DescriptorPool {VK_NULL_HANDLE};

  std::unique_ptr<VulkanSwapchain> m_Swapchain;
  SDL_Window* m_Window {nullptr};
  bool m_Initialized {false};
  bool m_ValidationEnabled {false};
  bool m_DepthEnabled {false};

  std::array<VulkanFrame, MAX_FRAMES_IN_FLIGHT> m_FrameData;
  std::vector<VkSemaphore> m_RenderFinishedSemaphores;
  uint32_t m_CurrentFrameIndex {0};
};

#endif
