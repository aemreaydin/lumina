#include <array>
#include <stdexcept>

#include "Renderer/RHI/Vulkan/VulkanImGui.hpp"

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>

#include "Core/Logger.hpp"
#include "Core/Window.hpp"
#include "Renderer/RHI/Vulkan/VulkanCommandBuffer.hpp"
#include "Renderer/RHI/Vulkan/VulkanDevice.hpp"
#include "Renderer/RHI/Vulkan/VulkanSwapchain.hpp"
#include "UI/ImGuiStyle.hpp"

VulkanImGui::VulkanImGui(VulkanDevice& device)
    : m_Device(device)
{
}

void VulkanImGui::Init(Window& window)
{
  Logger::Info("Initializing Vulkan ImGui backend");

  // Create descriptor pool for ImGui
  std::array<VkDescriptorPoolSize, 1> pool_sizes = {{
      {.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
       .descriptorCount = 100},
  }};

  VkDescriptorPoolCreateInfo pool_info {};
  pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
  pool_info.maxSets = 100;
  pool_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
  pool_info.pPoolSizes = pool_sizes.data();

  if (vkCreateDescriptorPool(
          m_Device.GetVkDevice(), &pool_info, nullptr, &m_DescriptorPool)
      != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create ImGui descriptor pool");
  }

  // Setup ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();

  ImGuiIO& imgui_io = ImGui::GetIO();
  imgui_io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

  // Load custom font with DPI scaling
  constexpr float BASE_FONT_SIZE = 15.0F;
  const float scale = window.GetDisplayScale();
  imgui_io.Fonts->AddFontFromFileTTF("fonts/InterVariable.ttf",
                                     BASE_FONT_SIZE * scale);

  // Apply flat theme and scale for HiDPI
  UIStyle::ApplyFlatTheme();
  ImGui::GetStyle().ScaleAllSizes(scale);

  // Initialize SDL3 backend
  auto* sdl_window = static_cast<SDL_Window*>(window.GetNativeWindow());
  ImGui_ImplSDL3_InitForVulkan(sdl_window);

  // Initialize Vulkan backend
  auto* swapchain = static_cast<VulkanSwapchain*>(m_Device.GetSwapchain());

  ImGui_ImplVulkan_InitInfo init_info {};
  init_info.ApiVersion = VulkanDevice::GetApiVersion();
  init_info.Instance = m_Device.GetVkInstance();
  init_info.PhysicalDevice = m_Device.GetVkPhysicalDevice();
  init_info.Device = m_Device.GetVkDevice();
  init_info.QueueFamily = m_Device.GetGraphicsQueueFamily();
  init_info.Queue = m_Device.GetGraphicsQueue();
  init_info.DescriptorPool = m_DescriptorPool;
  init_info.MinImageCount = swapchain->GetImageCount();
  init_info.ImageCount = swapchain->GetImageCount();
  init_info.UseDynamicRendering = true;

  // Dynamic rendering format - store in member to ensure pointer validity
  m_ColorFormat = swapchain->GetFormat();
  init_info.PipelineRenderingCreateInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
  init_info.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
  init_info.PipelineRenderingCreateInfo.pColorAttachmentFormats =
      &m_ColorFormat;

  ImGui_ImplVulkan_Init(&init_info);

  // Build font atlas and upload to GPU immediately so the texture is ready
  // before the first frame (avoids descriptor set issues during rendering)
  ImGui_ImplVulkan_CreateFontsTexture();

  Logger::Info("Vulkan ImGui backend initialized");
}

void VulkanImGui::Shutdown()
{
  Logger::Info("Shutting down Vulkan ImGui backend");

  m_Device.WaitIdle();

  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplSDL3_Shutdown();
  ImGui::DestroyContext();

  if (m_DescriptorPool != VK_NULL_HANDLE) {
    vkDestroyDescriptorPool(m_Device.GetVkDevice(), m_DescriptorPool, nullptr);
    m_DescriptorPool = VK_NULL_HANDLE;
  }
}

void VulkanImGui::BeginFrame()
{
  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplSDL3_NewFrame();
  ImGui::NewFrame();
}

void VulkanImGui::EndFrame()
{
  ImGui::Render();

  auto* cmd_buffer =
      static_cast<VulkanCommandBuffer*>(m_Device.GetCurrentCommandBuffer());

  ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(),
                                  cmd_buffer->GetHandle());
}
