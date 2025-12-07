#include <stdexcept>
#include <vector>

#include "Renderer/RHI/Vulkan/VulkanDevice.hpp"

#include <GLFW/glfw3.h>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_structs.hpp>

#include "Core/Logger.hpp"
#include "Renderer/RHI/RenderPassInfo.hpp"
#include "Renderer/RHI/Vulkan/VulkanCommandBuffer.hpp"
#include "Renderer/RendererConfig.hpp"

/*
 * TODO: There is no check of old swapchains or any cleanups of it
 * TODO: There should be checks in place for extension availability and
 * alternatives need to be implemented.
 */
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

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

  VULKAN_HPP_DEFAULT_DISPATCHER.init();

  m_Window = static_cast<GLFWwindow*>(window);
  m_ValidationEnabled = config.EnableValidation;

  vk::ApplicationInfo app_info {};
  app_info.pApplicationName = "Lumina";
  app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  app_info.pEngineName = "Lumina Engine";
  app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  app_info.apiVersion = VK_API_VERSION_1_3;

  uint32_t glfw_extension_count = 0;
  const char** glfw_extensions =
      glfwGetRequiredInstanceExtensions(&glfw_extension_count);

  const std::span extensions_view {glfw_extensions, glfw_extension_count};
  std::vector<const char*> extensions(extensions_view.begin(),
                                      extensions_view.end());

  // Add debug utils extension if validation is enabled
  if (m_ValidationEnabled) {
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }
  extensions.push_back(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME);
  extensions.push_back(VK_EXT_SURFACE_MAINTENANCE_1_EXTENSION_NAME);

  const std::vector<const char*> validation_layers = {
      "VK_LAYER_KHRONOS_validation"};

  vk::DebugUtilsMessengerCreateInfoEXT debug_create_info {};
  if (m_ValidationEnabled) {
    debug_create_info.messageSeverity =
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose
        | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo
        | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
        | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
    debug_create_info.messageType =
        vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral
        | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation
        | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;
    debug_create_info.pfnUserCallback = DebugCallback;
  }

  vk::InstanceCreateInfo create_info {};
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

  m_Instance = vk::createInstance(create_info);
  VULKAN_HPP_DEFAULT_DISPATCHER.init(m_Instance);

  // Set up debug messenger
  if (m_ValidationEnabled) {
    setup_debug_messenger();
  }

  Logger::Info("Vulkan instance created successfully");

  // Create surface
  VkSurfaceKHR surface_c = VK_NULL_HANDLE;
  if (glfwCreateWindowSurface(m_Instance, m_Window, nullptr, &surface_c)
      != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create window surface");
  }
  m_Surface = vk::SurfaceKHR(surface_c);

  // Pick physical device with surface support
  pick_physical_device(m_Surface);

  // Create logical device
  create_logical_device(m_Surface);

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

  Logger::Info("Vulkan swapchain created with {} images",
               m_Swapchain->GetImageCount());
}

void VulkanDevice::Destroy()
{
  if (!m_Initialized) {
    return;
  }

  if (m_Device) {
    m_Device.waitIdle();
  }

  for (const auto& frame : m_FrameData) {
    recycle_fence(frame.InFlightFence);
    recycle_semaphore(frame.ImageAvailableSemaphore);
    m_Device.destroyCommandPool(frame.CommandPool);
  }

  for (const auto& present_info : m_PresentInfo) {
    if (present_info.PresentFence != nullptr) {
      if (const auto result = m_Device.waitForFences(
              present_info.PresentFence, vk::True, UINT64_MAX);
          result != vk::Result::eSuccess)
      {
        throw std::runtime_error("Failed to wait for fences during Destroy");
      }
    }
    cleanup_present_history();
  }
  for (const auto& semaphore : m_SemaphorePool) {
    Logger::Info("Destroying semaphore {}", static_cast<void*>(semaphore));
    m_Device.destroySemaphore(semaphore);
  }
  for (const auto& fence : m_FencePool) {
    m_Device.destroyFence(fence);
  }

  m_Swapchain.reset();

  if (m_Device) {
    m_Device.destroy();
  }

  if (m_Surface) {
    m_Instance.destroySurfaceKHR(m_Surface);
  }

  if (m_ValidationEnabled && m_DebugMessenger) {
    m_Instance.destroyDebugUtilsMessengerEXT(m_DebugMessenger);
  }

  if (m_Instance) {
    m_Instance.destroy();
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

  RenderPassInfo render_pass {};
  render_pass.ColorAttachment.ColorLoadOp = LoadOp::Clear;
  render_pass.ColorAttachment.ClearColor = {
      .R = 1.0F, .G = 0.1F, .B = 0.1F, .A = 1.0F};
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

  const auto wait_stage_mask = vk::PipelineStageFlags {
      vk::PipelineStageFlagBits::eColorAttachmentOutput};

  vk::SubmitInfo submit_info {};
  auto cmd_buffer = frame_data.CommandBuffer.GetHandle();
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &cmd_buffer;
  submit_info.waitSemaphoreCount = 1;
  submit_info.pWaitSemaphores = &frame_data.ImageAvailableSemaphore;
  submit_info.pWaitDstStageMask = &wait_stage_mask;
  submit_info.signalSemaphoreCount = 1;
  submit_info.pSignalSemaphores = &frame_data.RenderFinishedSemaphore;

  Logger::Trace("[Vulkan] Submitting command buffer to graphics queue");
  const auto submit_result =
      m_GraphicsQueue.submit(1, &submit_info, frame_data.InFlightFence);
  if (submit_result != vk::Result::eSuccess) {
    throw std::runtime_error("Failed to submit to graphics queue");
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

  auto& frame_data = m_FrameData.at(m_CurrentFrameIndex);
  const auto current_index = m_Swapchain->GetCurrentImageIndex();

  const auto present_fence = get_fence();
  vk::SwapchainPresentFenceInfoEXT fence_info;
  fence_info.swapchainCount = 1;
  fence_info.pFences = &present_fence;

  vk::PresentInfoKHR present_info {};
  present_info.waitSemaphoreCount = 1;
  present_info.pWaitSemaphores = &frame_data.RenderFinishedSemaphore;
  present_info.swapchainCount = 1;
  auto swapchain_khr = m_Swapchain->GetVkSwapchain();
  present_info.pSwapchains = &swapchain_khr;
  present_info.pImageIndices = &current_index;
  present_info.pNext = &fence_info;

  auto present_result = m_PresentQueue.presentKHR(present_info);
  if (present_result == vk::Result::eErrorOutOfDateKHR
      || present_result == vk::Result::eSuboptimalKHR)
  {
    Logger::Trace("[Vulkan] Swapchain suboptimal or out of date, resizing");
    m_Swapchain->Resize(m_Swapchain->GetWidth(), m_Swapchain->GetHeight());
  } else if (present_result != vk::Result::eSuccess) {
    throw std::runtime_error("Failed to present swapchain image");
  }

  m_PresentInfo.emplace_back(frame_data.RenderFinishedSemaphore, present_fence);
  cleanup_present_history();

  m_CurrentFrameIndex = (m_CurrentFrameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
  Logger::Trace("[Vulkan] Advanced to frame {}", m_CurrentFrameIndex);
}

auto VulkanDevice::GetSwapchain() -> RHISwapchain*
{
  return m_Swapchain.get();
}

void VulkanDevice::pick_physical_device(vk::SurfaceKHR surface)
{
  const std::vector<vk::PhysicalDevice> devices =
      m_Instance.enumeratePhysicalDevices();

  if (devices.empty()) {
    throw std::runtime_error("Failed to find GPUs with Vulkan support");
  }

  // Check each device for surface support
  for (const auto& device : devices) {
    // Check if device supports presenting to the surface
    const std::vector<vk::QueueFamilyProperties> queue_families =
        device.getQueueFamilyProperties();

    bool has_surface_support = false;
    for (uint32_t i = 0; i < queue_families.size(); ++i) {
      const VkBool32 present_support = device.getSurfaceSupportKHR(i, surface);
      if (present_support != 0U) {
        has_surface_support = true;
        break;
      }
    }

    if (!has_surface_support) {
      continue;
    }

    // Prefer discrete GPU
    const vk::PhysicalDeviceProperties properties = device.getProperties();
    if (properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
      m_PhysicalDevice = device;
      Logger::Info("Selected discrete GPU: {}", properties.deviceName.data());
      return;
    }
  }

  // If no discrete GPU found, use first device with surface support
  for (const auto& device : devices) {
    const std::vector<vk::QueueFamilyProperties> queue_families =
        device.getQueueFamilyProperties();

    for (uint32_t i = 0; i < queue_families.size(); ++i) {
      const VkBool32 present_support = device.getSurfaceSupportKHR(i, surface);
      if (present_support != 0U) {
        m_PhysicalDevice = device;
        const vk::PhysicalDeviceProperties properties = device.getProperties();
        Logger::Info("Selected GPU: {}", properties.deviceName.data());
        return;
      }
    }
  }

  throw std::runtime_error(
      "Failed to find GPU with surface presentation support");
}

void VulkanDevice::create_logical_device(vk::SurfaceKHR surface)
{
  const std::vector<vk::QueueFamilyProperties> queue_families =
      m_PhysicalDevice.getQueueFamilyProperties();

  // Find queue families that support both graphics and present
  bool found = false;
  for (uint32_t i = 0; i < queue_families.size(); ++i) {
    const bool graphics =
        (queue_families[i].queueFlags & vk::QueueFlagBits::eGraphics)
        != vk::QueueFlags {};
    const VkBool32 present_support =
        m_PhysicalDevice.getSurfaceSupportKHR(i, surface);

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
  vk::DeviceQueueCreateInfo queue_create_info {};
  queue_create_info.queueFamilyIndex = m_GraphicsQueueFamily;
  queue_create_info.queueCount = 1;
  queue_create_info.pQueuePriorities = &queue_priority;

  const vk::PhysicalDeviceFeatures device_features {};

  vk::PhysicalDeviceSwapchainMaintenance1FeaturesEXT maintenance1_features {};
  maintenance1_features.swapchainMaintenance1 = VK_TRUE;

  vk::PhysicalDeviceVulkan13Features vulkan13_features {};
  vulkan13_features.dynamicRendering = VK_TRUE;
  vulkan13_features.synchronization2 = VK_TRUE;
  vulkan13_features.pNext = &maintenance1_features;

  const std::vector<const char*> device_extensions = {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME,
      VK_EXT_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME};

  vk::DeviceCreateInfo create_info {};
  create_info.pNext = &vulkan13_features;
  create_info.queueCreateInfoCount = 1;
  create_info.pQueueCreateInfos = &queue_create_info;
  create_info.pEnabledFeatures = &device_features;
  create_info.enabledExtensionCount =
      static_cast<uint32_t>(device_extensions.size());
  create_info.ppEnabledExtensionNames = device_extensions.data();

  m_Device = m_PhysicalDevice.createDevice(create_info);
  VULKAN_HPP_DEFAULT_DISPATCHER.init(m_Device);

  m_GraphicsQueue = m_Device.getQueue(m_GraphicsQueueFamily, 0);
  m_PresentQueue = m_Device.getQueue(m_PresentQueueFamily, 0);
}

void VulkanDevice::setup_debug_messenger()
{
  vk::DebugUtilsMessengerCreateInfoEXT create_info {};
  create_info.messageSeverity =
      vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose
      | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo
      | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
      | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
  create_info.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral
      | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation
      | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;
  create_info.pfnUserCallback = DebugCallback;

  m_DebugMessenger = m_Instance.createDebugUtilsMessengerEXT(create_info);
}

auto VulkanDevice::create_command_pool() -> vk::CommandPool
{
  vk::CommandPoolCreateInfo pool_info {};
  pool_info.flags = vk::CommandPoolCreateFlagBits::eTransient;
  pool_info.queueFamilyIndex = m_GraphicsQueueFamily;

  return m_Device.createCommandPool(pool_info);
}

void VulkanDevice::setup_frame_data()
{
  auto& frame_data = m_FrameData.at(m_CurrentFrameIndex);
  if (frame_data.InFlightFence != nullptr) {
    if (auto result = m_Device.waitForFences(
            frame_data.InFlightFence, vk::True, UINT64_MAX);
        result != vk::Result::eSuccess)
    {
      throw std::runtime_error("Failed to wait for fence.");
    }
    recycle_fence(frame_data.InFlightFence);
    recycle_semaphore(frame_data.ImageAvailableSemaphore);

    m_Device.resetCommandPool(frame_data.CommandPool);
  }

  frame_data.ImageAvailableSemaphore = get_semaphore();
  frame_data.RenderFinishedSemaphore = get_semaphore();
  frame_data.InFlightFence = get_fence();
  if (frame_data.CommandPool == nullptr) {
    frame_data.CommandPool = create_command_pool();
    frame_data.CommandBuffer.Allocate(*this, frame_data.CommandPool);
  }
}

auto VulkanDevice::get_fence() -> vk::Fence
{
  if (!m_FencePool.empty()) {
    const auto fence = m_FencePool.back();
    m_FencePool.pop_back();
    return fence;
  }

  const vk::FenceCreateInfo fence_info {};
  return m_Device.createFence(fence_info);
}

void VulkanDevice::recycle_fence(vk::Fence fence)
{
  m_FencePool.push_back(fence);
  auto reset_result = m_Device.resetFences(1, &fence);
  if (reset_result != vk::Result::eSuccess) {
    throw std::runtime_error("Failed to reset fence");
  }
}

auto VulkanDevice::get_semaphore() -> vk::Semaphore
{
  if (!m_SemaphorePool.empty()) {
    const auto semaphore = m_SemaphorePool.back();
    Logger::Info("Creating a new semaphore {}", static_cast<void*>(semaphore));
    m_SemaphorePool.pop_back();
    return semaphore;
  }

  return m_Device.createSemaphore({});
}

void VulkanDevice::recycle_semaphore(vk::Semaphore semaphore)
{
  Logger::Info("Recycling semaphore {}", static_cast<void*>(semaphore));
  m_SemaphorePool.push_back(semaphore);
}

void VulkanDevice::cleanup_present_history()
{
  while (!m_PresentInfo.empty()) {
    const auto present_info = m_PresentInfo.front();
    const auto result = m_Device.getFenceStatus(present_info.PresentFence);
    if (result == vk::Result::eNotReady) {
      break;
    }
    if (result != vk::Result::eSuccess) {
      throw std::runtime_error("Failed to get fence status");
    }
    m_PresentInfo.pop_front();
    if (present_info.PresentFence != nullptr) {
      recycle_fence(present_info.PresentFence);
      Logger::Info("Recycling present fence");
    }
    if (present_info.PresentSemaphore != nullptr) {
      recycle_semaphore(present_info.PresentSemaphore);
      Logger::Info("Recycling present semaphore");
    }
  }

  // The present history can grow indefinitely if a present operation is done on
  // an index that's never acquired in the future.  In that case, there's no
  // fence associated with that present operation.  Move the offending entry to
  // last, so the resources associated with the rest of the present operations
  // can be duly freed.
  // if (present_history.size() > swapchain_objects.images.size() * 2
  //     && present_history.front().cleanup_fence == VK_NULL_HANDLE)
  // {
  //   PresentOperationInfo present_info = std::move(present_history.front());
  //   present_history.pop_front();
  //
  //   // We can't be stuck on a presentation to an old swapchain without a
  //   fence. assert(present_info.image_index != INVALID_IMAGE_INDEX);
  //
  //   // Move clean up data to the next (now first) present operation, if any.
  //   // Note that there cannot be any clean up data on the rest of the present
  //   // operations, because the first present already gathers every old
  //   swapchain
  //   // to clean up.
  //   assert(std::ranges::all_of(present_history,
  //                              [](const PresentOperationInfo& op)
  //                              { return op.old_swapchains.empty(); }));
  //   present_history.front().old_swapchains =
  //       std::move(present_info.old_swapchains);
  //
  //   // Put the present operation at the end of the queue, so it's revisited
  //   // after the rest of the present operations are cleaned up.
  //   present_history.push_back(std::move(present_info));
  // }
}
