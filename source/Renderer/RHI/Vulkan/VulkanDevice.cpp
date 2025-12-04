#include <stdexcept>
#include <vector>

#include "Renderer/RHI/Vulkan/VulkanDevice.hpp"

#include <GLFW/glfw3.h>

#include "Core/Logger.hpp"
#include "Renderer/RendererConfig.hpp"

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

  m_Initialized = true;
}

void VulkanDevice::CreateSwapchain([[maybe_unused]] uint32_t width,
                                   [[maybe_unused]] uint32_t height)
{
  if (m_Window == nullptr) {
    throw std::runtime_error("Device not initialized with window");
  }

  m_Swapchain = std::make_unique<VulkanSwapchain>(*this, m_Window);
  Logger::Info("Vulkan swapchain created");
}

void VulkanDevice::Destroy()
{
  if (!m_Initialized) {
    return;
  }

  // Wait for device to finish all operations
  if (m_Device) {
    m_Device.waitIdle();
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
  // Acquire next swapchain image
  if (m_Swapchain != nullptr) {
    m_Swapchain->AcquireNextImage();
  }
}

void VulkanDevice::EndFrame()
{
  // Nothing to do here for now
}

void VulkanDevice::Present()
{
  if (m_Swapchain != nullptr) {
    m_Swapchain->Present(0);
  }
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

  // Enable dynamic rendering (Vulkan 1.3)
  vk::PhysicalDeviceDynamicRenderingFeatures dynamic_rendering_features {};
  dynamic_rendering_features.dynamicRendering = VK_TRUE;

  vk::PhysicalDeviceVulkan13Features vulkan13_features {};
  vulkan13_features.dynamicRendering = VK_TRUE;
  vulkan13_features.synchronization2 = VK_TRUE;

  const std::vector<const char*> device_extensions = {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME};

  vk::DeviceCreateInfo create_info {};
  create_info.pNext = &vulkan13_features;
  create_info.queueCreateInfoCount = 1;
  create_info.pQueueCreateInfos = &queue_create_info;
  create_info.pEnabledFeatures = &device_features;
  create_info.enabledExtensionCount =
      static_cast<uint32_t>(device_extensions.size());
  create_info.ppEnabledExtensionNames = device_extensions.data();

  m_Device = m_PhysicalDevice.createDevice(create_info);

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
