#include <format>
#include <stdexcept>
#include <vector>

#include "Renderer/RHI/Vulkan/VulkanDevice.hpp"

#include <SDL3/SDL_vulkan.h>

#include "Core/Logger.hpp"
#include "Renderer/RHI/RHIBuffer.hpp"
#include "Renderer/RHI/RHIDescriptorSet.hpp"
#include "Renderer/RHI/RHIPipeline.hpp"
#include "Renderer/RHI/RHIShaderModule.hpp"
#include "Renderer/RHI/RenderPassInfo.hpp"
#include "Renderer/RHI/Vulkan/VulkanBuffer.hpp"
#include "Renderer/RHI/Vulkan/VulkanCommandBuffer.hpp"
#include "Renderer/RHI/Vulkan/VulkanDescriptorSet.hpp"
#include "Renderer/RHI/Vulkan/VulkanPipelineLayout.hpp"
#include "Renderer/RHI/Vulkan/VulkanSampler.hpp"
#include "Renderer/RHI/Vulkan/VulkanShaderModule.hpp"
#include "Renderer/RHI/Vulkan/VulkanTexture.hpp"
#include "Renderer/RHI/Vulkan/VulkanUtils.hpp"
#include "Renderer/RendererConfig.hpp"

/*
 * TODO: There should be checks in place for extension availability and
 * alternatives need to be implemented.
 */

static VKAPI_ATTR auto VKAPI_CALL
DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
              VkDebugUtilsMessageTypeFlagsEXT /*message_type*/,
              const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
              void* /*user_data*/) -> VkBool32
{
  switch (message_severity) {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
      Logger::Trace("[Vulkan] {}", callback_data->pMessage);
      break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
      Logger::Info("[Vulkan] {}", callback_data->pMessage);
      break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
      Logger::Warn("[Vulkan] {}", callback_data->pMessage);
      break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
      Logger::Error("[Vulkan] {}", callback_data->pMessage);
      break;
    default:
      break;
  }
  return VK_FALSE;
}

void VulkanDevice::Init(const RendererConfig& config, void* window)
{
  if (m_Initialized) {
    return;
  }

  if (window == nullptr) {
    throw std::runtime_error("Window pointer is null");
  }

  m_Window = static_cast<SDL_Window*>(window);
  m_ValidationEnabled = config.EnableValidation;
  m_DepthEnabled = config.EnableDepth;

  // Initialize volk
  if (auto result = VkUtils::Check(volkInitialize()); !result) {
    throw std::runtime_error(std::format("Failed to initialize volk: {}",
                                         VkUtils::ToString(result.error())));
  }

  VkApplicationInfo app_info = {};
  app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  app_info.pApplicationName = "Lumina";
  app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  app_info.pEngineName = "Lumina Engine";
  app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  app_info.apiVersion = VK_API_VERSION_1_3;

  // Get required Vulkan instance extensions from SDL
  Uint32 sdl_extension_count = 0;
  const char* const* sdl_extensions =
      SDL_Vulkan_GetInstanceExtensions(&sdl_extension_count);

  const std::span extensions_view {sdl_extensions, sdl_extension_count};
  std::vector<const char*> extensions(extensions_view.begin(),
                                      extensions_view.end());

  // Add debug utils extension if validation is enabled
  if (m_ValidationEnabled) {
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }

  const std::vector<const char*> validation_layers = {
      "VK_LAYER_KHRONOS_validation"};

  VkDebugUtilsMessengerCreateInfoEXT debug_create_info = {};
  if (m_ValidationEnabled) {
    debug_create_info.sType =
        VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debug_create_info.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debug_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debug_create_info.pfnUserCallback = DebugCallback;
  }

  VkInstanceCreateInfo create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  create_info.pApplicationInfo = &app_info;
  create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  create_info.ppEnabledExtensionNames = extensions.data();

  if (m_ValidationEnabled) {
    create_info.enabledLayerCount =
        static_cast<uint32_t>(validation_layers.size());
    create_info.ppEnabledLayerNames = validation_layers.data();
    create_info.pNext = &debug_create_info;
  } else {
    create_info.enabledLayerCount = 0;
  }

  if (auto result =
          VkUtils::Check(vkCreateInstance(&create_info, nullptr, &m_Instance));
      !result)
  {
    throw std::runtime_error(std::format("Failed to create Vulkan instance: {}",
                                         VkUtils::ToString(result.error())));
  }

  // Load instance-level functions
  volkLoadInstance(m_Instance);

  // Set up debug messenger
  if (m_ValidationEnabled) {
    setup_debug_messenger();
  }

  Logger::Info("Vulkan instance created successfully");

  // Create surface using SDL
  if (!SDL_Vulkan_CreateSurface(m_Window, m_Instance, nullptr, &m_Surface)) {
    throw std::runtime_error(
        std::format("Failed to create window surface: {}", SDL_GetError()));
  }

  // Pick physical device with surface support
  pick_physical_device(m_Surface);

  // Create logical device
  create_logical_device(m_Surface);

  // Create descriptor pool
  create_descriptor_pool();

  Logger::Info("Created synchronization primitives for {} frames in flight",
               MAX_FRAMES_IN_FLIGHT);

  m_Initialized = true;
}

void VulkanDevice::CreateSwapchain([[maybe_unused]] uint32_t width,
                                   [[maybe_unused]] uint32_t height)
{
  if (m_Window == nullptr) {
    throw std::runtime_error("Device not initialized with window");
  }

  m_Swapchain = std::make_unique<VulkanSwapchain>(*this, m_Window);

  m_RenderFinishedSemaphores.resize(m_Swapchain->GetImageCount());
  for (auto& semaphore : m_RenderFinishedSemaphores) {
    VkSemaphoreCreateInfo semaphore_info = {};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    if (auto result = VkUtils::Check(
            vkCreateSemaphore(m_Device, &semaphore_info, nullptr, &semaphore));
        !result)
    {
      throw std::runtime_error(
          std::format("Failed to create render finished semaphore: {}",
                      VkUtils::ToString(result.error())));
    }
  }

  Logger::Info("Vulkan swapchain created with {} images",
               m_Swapchain->GetImageCount());
}

void VulkanDevice::Destroy()
{
  if (!m_Initialized) {
    return;
  }

  WaitIdle();

  for (const auto& frame : m_FrameData) {
    vkDestroySemaphore(m_Device, frame.ImageAvailableSemaphore, nullptr);
    vkDestroyFence(m_Device, frame.InFlightFence, nullptr);
    vkDestroyCommandPool(m_Device, frame.CommandPool, nullptr);
  }

  for (const auto& semaphore : m_RenderFinishedSemaphores) {
    vkDestroySemaphore(m_Device, semaphore, nullptr);
  }

  m_Swapchain.reset();

  if (m_DescriptorPool != VK_NULL_HANDLE) {
    vkDestroyDescriptorPool(m_Device, m_DescriptorPool, nullptr);
  }

  if (m_Device != VK_NULL_HANDLE) {
    vkDestroyDevice(m_Device, nullptr);
  }

  if (m_Surface != VK_NULL_HANDLE) {
    vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
  }

  if (m_ValidationEnabled && m_DebugMessenger != VK_NULL_HANDLE) {
    vkDestroyDebugUtilsMessengerEXT(m_Instance, m_DebugMessenger, nullptr);
  }

  if (m_Instance != VK_NULL_HANDLE) {
    vkDestroyInstance(m_Instance, nullptr);
  }

  m_Initialized = false;
}

void VulkanDevice::BeginFrame()
{
  if (!m_Swapchain) {
    return;
  }

  Logger::Trace("[Vulkan] Begin frame {}", m_CurrentFrameIndex);

  setup_frame_data();
  auto& frame_data = m_FrameData.at(m_CurrentFrameIndex);

  m_Swapchain->AcquireNextImage(frame_data.ImageAvailableSemaphore);

  frame_data.CommandBuffer.Begin();

  DepthStencilInfo depth_stencil {};
  depth_stencil.DepthLoadOp = LoadOp::Clear;
  depth_stencil.DepthStoreOp = StoreOp::DontCare;
  depth_stencil.ClearDepthStencil.Depth = 1.0F;

  RenderPassInfo render_pass {};
  render_pass.ColorAttachment.ColorLoadOp = LoadOp::Clear;
  render_pass.ColorAttachment.ClearColor = {
      .R = 0.1F, .G = 0.1F, .B = 0.1F, .A = 1.0F};
  if (m_DepthEnabled) {
    render_pass.DepthStencilAttachment = &depth_stencil;
  }
  render_pass.Width = m_Swapchain->GetWidth();
  render_pass.Height = m_Swapchain->GetHeight();

  frame_data.CommandBuffer.BeginRenderPass(*m_Swapchain, render_pass);
}

void VulkanDevice::EndFrame()
{
  if (!m_Swapchain) {
    return;
  }

  Logger::Trace("[Vulkan] End frame {}", m_CurrentFrameIndex);

  auto& frame_data = m_FrameData.at(m_CurrentFrameIndex);
  frame_data.CommandBuffer.EndRenderPass(*m_Swapchain);
  frame_data.CommandBuffer.End();

  const VkPipelineStageFlags wait_stage_mask =
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

  VkSubmitInfo submit_info = {};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  VkCommandBuffer cmd_buffer = frame_data.CommandBuffer.GetHandle();
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &cmd_buffer;
  submit_info.waitSemaphoreCount = 1;
  submit_info.pWaitSemaphores = &frame_data.ImageAvailableSemaphore;
  submit_info.pWaitDstStageMask = &wait_stage_mask;
  submit_info.signalSemaphoreCount = 1;
  submit_info.pSignalSemaphores =
      &m_RenderFinishedSemaphores.at(m_Swapchain->GetCurrentImageIndex());

  Logger::Trace("[Vulkan] Submitting command buffer to graphics queue");
  if (auto result = VkUtils::Check(vkQueueSubmit(
          m_GraphicsQueue, 1, &submit_info, frame_data.InFlightFence));
      !result)
  {
    throw std::runtime_error(
        std::format("Failed to submit to graphics queue: {}",
                    VkUtils::ToString(result.error())));
  }
}

void VulkanDevice::Present()
{
  if (!m_Swapchain) {
    return;
  }

  Logger::Trace("[Vulkan] Present image {} from frame {}",
                m_Swapchain->GetCurrentImageIndex(),
                m_CurrentFrameIndex);

  const uint32_t current_index = m_Swapchain->GetCurrentImageIndex();

  VkPresentInfoKHR present_info = {};
  present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  present_info.waitSemaphoreCount = 1;
  present_info.pWaitSemaphores =
      &m_RenderFinishedSemaphores.at(m_Swapchain->GetCurrentImageIndex());
  present_info.swapchainCount = 1;
  VkSwapchainKHR swapchain_khr = m_Swapchain->GetVkSwapchain();
  present_info.pSwapchains = &swapchain_khr;
  present_info.pImageIndices = &current_index;

  const auto allowed = {VK_SUBOPTIMAL_KHR, VK_ERROR_OUT_OF_DATE_KHR};
  auto result =
      VkUtils::Check(vkQueuePresentKHR(m_PresentQueue, &present_info), allowed);
  if (result
      && (result.value() == VK_SUBOPTIMAL_KHR
          || result.value() == VK_ERROR_OUT_OF_DATE_KHR))
  {
    Logger::Trace("[Vulkan] Swapchain suboptimal or out of date, resizing");
    m_Swapchain->Resize(m_Swapchain->GetWidth(), m_Swapchain->GetHeight());
  } else if (!result) {
    {
      throw std::runtime_error(
          std::format("Failed to present swapchain image: {}",
                      VkUtils::ToString(result.error())));
    }
  }

  m_CurrentFrameIndex = (m_CurrentFrameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
  Logger::Trace("[Vulkan] Advanced to frame {}", m_CurrentFrameIndex);
}

auto VulkanDevice::GetSwapchain() -> RHISwapchain*
{
  return m_Swapchain.get();
}

void VulkanDevice::WaitIdle()
{
  if (m_Device != VK_NULL_HANDLE) {
    vkDeviceWaitIdle(m_Device);
  }
}

void VulkanDevice::pick_physical_device(VkSurfaceKHR surface)
{
  uint32_t device_count = 0;
  vkEnumeratePhysicalDevices(m_Instance, &device_count, nullptr);

  if (device_count == 0) {
    throw std::runtime_error("Failed to find GPUs with Vulkan support");
  }

  std::vector<VkPhysicalDevice> devices(device_count);
  if (auto result = VkUtils::Check(vkEnumeratePhysicalDevices(
          m_Instance, &device_count, devices.data()));
      !result)
  {
    throw std::runtime_error(
        std::format("Failed to enumerate physical devices: {}",
                    VkUtils::ToString(result.error())));
  }

  // Check each device for surface support
  for (const auto& device : devices) {
    // Check if device supports presenting to the surface
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(
        device, &queue_family_count, nullptr);
    std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(
        device, &queue_family_count, queue_families.data());

    bool has_surface_support = false;
    for (uint32_t i = 0; i < queue_families.size(); ++i) {
      VkBool32 present_support = VK_FALSE;
      vkGetPhysicalDeviceSurfaceSupportKHR(
          device, i, surface, &present_support);
      if (present_support != 0U) {
        has_surface_support = true;
        break;
      }
    }

    if (!has_surface_support) {
      continue;
    }

    // Prefer discrete GPU
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(device, &properties);
    if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
      m_PhysicalDevice = device;
      Logger::Info("Selected discrete GPU: {}", properties.deviceName);
      return;
    }
  }

  // If no discrete GPU found, use first device with surface support
  for (const auto& device : devices) {
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(
        device, &queue_family_count, nullptr);
    std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(
        device, &queue_family_count, queue_families.data());

    for (uint32_t i = 0; i < queue_families.size(); ++i) {
      VkBool32 present_support = VK_FALSE;
      vkGetPhysicalDeviceSurfaceSupportKHR(
          device, i, surface, &present_support);
      if (present_support != 0U) {
        m_PhysicalDevice = device;
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(device, &properties);
        Logger::Info("Selected GPU: {}", properties.deviceName);
        return;
      }
    }
  }

  throw std::runtime_error(
      "Failed to find GPU with surface presentation support");
}

void VulkanDevice::create_logical_device(VkSurfaceKHR surface)
{
  uint32_t queue_family_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(
      m_PhysicalDevice, &queue_family_count, nullptr);
  std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
  vkGetPhysicalDeviceQueueFamilyProperties(
      m_PhysicalDevice, &queue_family_count, queue_families.data());

  // Find queue families that support both graphics and present
  bool found = false;
  for (uint32_t i = 0; i < queue_families.size(); ++i) {
    const bool graphics =
        (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0;
    VkBool32 present_support = VK_FALSE;
    vkGetPhysicalDeviceSurfaceSupportKHR(
        m_PhysicalDevice, i, surface, &present_support);

    if (graphics && (present_support != 0U)) {
      m_GraphicsQueueFamily = i;
      m_PresentQueueFamily = i;
      found = true;
      break;
    }
  }

  if (!found) {
    throw std::runtime_error(
        "Failed to find queue family with graphics and present support");
  }

  const float queue_priority = 1.0F;
  VkDeviceQueueCreateInfo queue_create_info = {};
  queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  queue_create_info.queueFamilyIndex = m_GraphicsQueueFamily;
  queue_create_info.queueCount = 1;
  queue_create_info.pQueuePriorities = &queue_priority;

  const VkPhysicalDeviceFeatures device_features = {};

  // Enable shader object extension features
  VkPhysicalDeviceShaderObjectFeaturesEXT shader_object_features = {};
  shader_object_features.sType =
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_OBJECT_FEATURES_EXT;
  shader_object_features.shaderObject = VK_TRUE;

  // Enable dynamic rendering unused attachments to suppress false-positive
  // validation errors when using shader objects with depth attachments
  VkPhysicalDeviceDynamicRenderingUnusedAttachmentsFeaturesEXT
      unused_attachments_features = {};
  unused_attachments_features.sType =
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_UNUSED_ATTACHMENTS_FEATURES_EXT;
  unused_attachments_features.pNext = &shader_object_features;
  unused_attachments_features.dynamicRenderingUnusedAttachments = VK_TRUE;

  VkPhysicalDeviceVulkan13Features vulkan13_features = {};
  vulkan13_features.sType =
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
  vulkan13_features.pNext = &unused_attachments_features;
  vulkan13_features.dynamicRendering = VK_TRUE;
  vulkan13_features.synchronization2 = VK_TRUE;

  const std::vector<const char*> device_extensions = {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME,
      VK_EXT_SHADER_OBJECT_EXTENSION_NAME,
      VK_EXT_DYNAMIC_RENDERING_UNUSED_ATTACHMENTS_EXTENSION_NAME,
  };

  VkDeviceCreateInfo create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  create_info.pNext = &vulkan13_features;
  create_info.queueCreateInfoCount = 1;
  create_info.pQueueCreateInfos = &queue_create_info;
  create_info.pEnabledFeatures = &device_features;
  create_info.enabledExtensionCount =
      static_cast<uint32_t>(device_extensions.size());
  create_info.ppEnabledExtensionNames = device_extensions.data();

  if (auto result = VkUtils::Check(
          vkCreateDevice(m_PhysicalDevice, &create_info, nullptr, &m_Device));
      !result)
  {
    throw std::runtime_error(std::format("Failed to create logical device: {}",
                                         VkUtils::ToString(result.error())));
  }

  // Load device-level functions
  volkLoadDevice(m_Device);

  vkGetDeviceQueue(m_Device, m_GraphicsQueueFamily, 0, &m_GraphicsQueue);
  vkGetDeviceQueue(m_Device, m_PresentQueueFamily, 0, &m_PresentQueue);
}

void VulkanDevice::setup_debug_messenger()
{
  VkDebugUtilsMessengerCreateInfoEXT create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
      | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
      | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
      | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
      | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
      | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  create_info.pfnUserCallback = DebugCallback;

  if (auto result = VkUtils::Check(vkCreateDebugUtilsMessengerEXT(
          m_Instance, &create_info, nullptr, &m_DebugMessenger));
      !result)
  {
    throw std::runtime_error(std::format("Failed to create debug messenger: {}",
                                         VkUtils::ToString(result.error())));
  }
}

auto VulkanDevice::create_command_pool() -> VkCommandPool
{
  VkCommandPoolCreateInfo pool_info = {};
  pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  pool_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
  pool_info.queueFamilyIndex = m_GraphicsQueueFamily;

  VkCommandPool pool {};
  if (auto result = VkUtils::Check(
          vkCreateCommandPool(m_Device, &pool_info, nullptr, &pool));
      !result)
  {
    throw std::runtime_error(std::format("Failed to create command pool: {}",
                                         VkUtils::ToString(result.error())));
  }
  return pool;
}

void VulkanDevice::setup_frame_data()
{
  auto& frame_data = m_FrameData.at(m_CurrentFrameIndex);
  if (frame_data.InFlightFence != VK_NULL_HANDLE) {
    if (auto result = VkUtils::Check(vkWaitForFences(
            m_Device, 1, &frame_data.InFlightFence, VK_TRUE, UINT64_MAX));
        !result)
    {
      throw std::runtime_error(std::format("Failed to wait for fence: {}",
                                           VkUtils::ToString(result.error())));
    }

    if (auto result = VkUtils::Check(
            vkResetFences(m_Device, 1, &frame_data.InFlightFence));
        !result)
    {
      throw std::runtime_error(std::format("Failed to reset fence: {}",
                                           VkUtils::ToString(result.error())));
    }

    if (auto result = VkUtils::Check(
            vkResetCommandPool(m_Device, frame_data.CommandPool, 0));
        !result)
    {
      throw std::runtime_error(std::format("Failed to reset command pool: {}",
                                           VkUtils::ToString(result.error())));
    }
  }

  if (frame_data.InFlightFence == VK_NULL_HANDLE) {
    VkFenceCreateInfo fence_info = {};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

    if (auto result = VkUtils::Check(vkCreateFence(
            m_Device, &fence_info, nullptr, &frame_data.InFlightFence));
        !result)
    {
      throw std::runtime_error(std::format("Failed to create fence: {}",
                                           VkUtils::ToString(result.error())));
    }
  }
  if (frame_data.ImageAvailableSemaphore == VK_NULL_HANDLE) {
    VkSemaphoreCreateInfo semaphore_info = {};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    if (auto result = VkUtils::Check(
            vkCreateSemaphore(m_Device,
                              &semaphore_info,
                              nullptr,
                              &frame_data.ImageAvailableSemaphore));
        !result)
    {
      throw std::runtime_error(std::format("Failed to create semaphore: {}",
                                           VkUtils::ToString(result.error())));
    }
  }
  if (frame_data.CommandPool == VK_NULL_HANDLE) {
    frame_data.CommandPool = create_command_pool();
    frame_data.CommandBuffer.Allocate(*this, frame_data.CommandPool);
  }
}

auto VulkanDevice::CreateBuffer(const BufferDesc& desc)
    -> std::unique_ptr<RHIBuffer>
{
  return std::make_unique<VulkanBuffer>(*this, desc);
}

auto VulkanDevice::CreateTexture(const TextureDesc& desc)
    -> std::unique_ptr<RHITexture>
{
  return std::make_unique<VulkanTexture>(*this, desc);
}

auto VulkanDevice::CreateSampler(const SamplerDesc& desc)
    -> std::unique_ptr<RHISampler>
{
  return std::make_unique<VulkanSampler>(*this, desc);
}

auto VulkanDevice::CreateShaderModule(const ShaderModuleDesc& desc)
    -> std::unique_ptr<RHIShaderModule>
{
  return std::make_unique<VulkanShaderModule>(*this, desc);
}

auto VulkanDevice::CreateGraphicsPipeline(
    [[maybe_unused]] const GraphicsPipelineDesc& desc)
    -> std::unique_ptr<RHIGraphicsPipeline>
{
  // With shader objects, we don't need traditional pipelines
  // TODO: Implement fallback VkPipeline for devices without
  // VK_EXT_shader_object
  return nullptr;
}

auto VulkanDevice::GetCurrentCommandBuffer() -> RHICommandBuffer*
{
  return &m_FrameData.at(m_CurrentFrameIndex).CommandBuffer;
}

void VulkanDevice::create_descriptor_pool()
{
  // Create a general-purpose descriptor pool
  // These limits can be adjusted based on actual needs
  const uint32_t pool_size = 100;
  std::array<VkDescriptorPoolSize, 4> pool_sizes = {{
      {.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = pool_size},
      {.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount = pool_size},
      {.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
       .descriptorCount = pool_size},
      {.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, .descriptorCount = pool_size},
  }};

  VkDescriptorPoolCreateInfo pool_info {};
  pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  pool_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
  pool_info.pPoolSizes = pool_sizes.data();
  pool_info.maxSets = pool_size;
  pool_info.flags = 0;

  if (auto result = VkUtils::Check(vkCreateDescriptorPool(
          m_Device, &pool_info, nullptr, &m_DescriptorPool));
      !result)
  {
    throw std::runtime_error(std::format("Failed to create descriptor pool: {}",
                                         VkUtils::ToString(result.error())));
  }

  Logger::Trace("[Vulkan] Created descriptor pool");
}

auto VulkanDevice::CreateDescriptorSetLayout(
    const DescriptorSetLayoutDesc& desc)
    -> std::shared_ptr<RHIDescriptorSetLayout>
{
  return std::make_shared<VulkanDescriptorSetLayout>(*this, desc);
}

auto VulkanDevice::CreateDescriptorSet(
    const std::shared_ptr<RHIDescriptorSetLayout>& layout)
    -> std::unique_ptr<RHIDescriptorSet>
{
  return std::make_unique<VulkanDescriptorSet>(*this, m_DescriptorPool, layout);
}

auto VulkanDevice::CreatePipelineLayout(const PipelineLayoutDesc& desc)
    -> std::shared_ptr<RHIPipelineLayout>
{
  return std::make_shared<VulkanPipelineLayout>(*this, desc);
}
