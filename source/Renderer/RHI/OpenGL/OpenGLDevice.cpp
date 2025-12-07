#include <stdexcept>

#include "Renderer/RHI/OpenGL/OpenGLDevice.hpp"

#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include "Core/Logger.hpp"
#include "Renderer/RHI/RenderPassInfo.hpp"
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

  // Create command buffer
  m_CommandBuffer = std::make_unique<RHICommandBuffer<OpenGLBackend>>();

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
  m_CommandBuffer.reset();
  m_Swapchain.reset();
  m_Initialized = false;
}

void OpenGLDevice::BeginFrame()
{
  if (!m_CommandBuffer || !m_Swapchain) {
    return;
  }

  m_CommandBuffer->Begin();

  RenderPassInfo render_pass {};
  render_pass.ColorAttachment.ColorLoadOp = LoadOp::Clear;
  render_pass.ColorAttachment.ClearColor = {
      .R = 1.0F, .G = 0.1F, .B = 0.1F, .A = 1.0F};
  render_pass.Width = m_Swapchain->GetWidth();
  render_pass.Height = m_Swapchain->GetHeight();

  m_CommandBuffer->BeginRenderPass(render_pass);
}

void OpenGLDevice::EndFrame()
{
  if (!m_CommandBuffer) {
    return;
  }

  m_CommandBuffer->EndRenderPass();
  m_CommandBuffer->End();
}

void OpenGLDevice::Present()
{
  if (m_Window != nullptr) {
    glfwSwapBuffers(m_Window);
  }
}

auto OpenGLDevice::GetSwapchain() -> RHISwapchain*
{
  return m_Swapchain.get();
}
