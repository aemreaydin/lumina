#include <stdexcept>

#include "Renderer/RHI/OpenGL/OpenGLDevice.hpp"

#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include "Core/Logger.hpp"
#include "Renderer/RendererConfig.hpp"

void OpenGLDevice::Init([[maybe_unused]] const RendererConfig& config,
                        void* window)
{
  if (m_Initialized) {
    return;
  }

  if (window == nullptr) {
    throw std::runtime_error("Window pointer is null");
  }

  m_Window = static_cast<GLFWwindow*>(window);
  Logger::Info("Initializing OpenGL device");

  // Make context current
  glfwMakeContextCurrent(m_Window);

  // Load OpenGL function pointers
  if (gladLoadGL() == 0) {
    Logger::Critical("Failed to load OpenGL function pointers");
    throw std::runtime_error("gladLoadGL failed");
  }

  Logger::Info("OpenGL Version: {}.{}", GLVersion.major, GLVersion.minor);

  // Enable VSync by default
  glfwSwapInterval(1);

  m_Initialized = true;
  Logger::Info("OpenGL device initialized successfully");
}

void OpenGLDevice::CreateSwapchain([[maybe_unused]] uint32_t width,
                                   [[maybe_unused]] uint32_t height)
{
  if (m_Window == nullptr) {
    throw std::runtime_error("Device not initialized with window");
  }

  m_Swapchain = std::make_unique<OpenGLSwapchain>(m_Window);
  Logger::Info("OpenGL swapchain created");
}

void OpenGLDevice::Destroy()
{
  if (!m_Initialized) {
    return;
  }

  Logger::Trace("OpenGL device shutting down");
  m_Swapchain.reset();
  m_Initialized = false;
}

void OpenGLDevice::BeginFrame()
{
  glClearColor(1.0F, 0.1F, 0.1F, 1.0F);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void OpenGLDevice::EndFrame()
{
  // Nothing to do here for OpenGL
}

void OpenGLDevice::Present()
{
  if (m_Swapchain != nullptr) {
    m_Swapchain->Present(0);
  }
}

auto OpenGLDevice::GetSwapchain() -> RHISwapchain*
{
  return m_Swapchain.get();
}
