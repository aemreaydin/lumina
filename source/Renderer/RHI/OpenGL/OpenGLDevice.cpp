#include <stdexcept>

#include "Renderer/RHI/OpenGL/OpenGLDevice.hpp"

#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include "Core/Logger.hpp"
#include "Renderer/RHI/OpenGL/OpenGLBuffer.hpp"
#include "Renderer/RHI/OpenGL/OpenGLDescriptorSet.hpp"
#include "Renderer/RHI/OpenGL/OpenGLPipelineLayout.hpp"
#include "Renderer/RHI/OpenGL/OpenGLShaderModule.hpp"
#include "Renderer/RHI/RHIBuffer.hpp"
#include "Renderer/RHI/RHIDescriptorSet.hpp"
#include "Renderer/RHI/RHIPipeline.hpp"
#include "Renderer/RHI/RHIShaderModule.hpp"
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

  // Enable sRGB framebuffer to match Vulkan's sRGB swapchain
  glEnable(GL_FRAMEBUFFER_SRGB);

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

  // Check for resize
  int width = 0;
  int height = 0;
  glfwGetFramebufferSize(m_Window, &width, &height);
  if (static_cast<uint32_t>(width) != m_Swapchain->GetWidth()
      || static_cast<uint32_t>(height) != m_Swapchain->GetHeight())
  {
    m_Swapchain->Resize(static_cast<uint32_t>(width),
                        static_cast<uint32_t>(height));
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

void OpenGLDevice::WaitIdle()
{
  glFinish();
}

auto OpenGLDevice::CreateBuffer(const BufferDesc& desc)
    -> std::unique_ptr<RHIBuffer>
{
  return std::make_unique<OpenGLBuffer>(desc);
}

auto OpenGLDevice::CreateShaderModule(const ShaderModuleDesc& desc)
    -> std::unique_ptr<RHIShaderModule>
{
  return std::make_unique<OpenGLShaderModule>(desc);
}

auto OpenGLDevice::CreateGraphicsPipeline(
    [[maybe_unused]] const GraphicsPipelineDesc& desc)
    -> std::unique_ptr<RHIGraphicsPipeline>
{
  // TODO: Implement OpenGL pipeline (shader program + VAO state)
  return nullptr;
}

void OpenGLDevice::BindShaders(const RHIShaderModule* vertex_shader,
                               const RHIShaderModule* fragment_shader)
{
  m_CommandBuffer->BindShaders(
      dynamic_cast<const OpenGLShaderModule*>(vertex_shader),
      dynamic_cast<const OpenGLShaderModule*>(fragment_shader));
}

void OpenGLDevice::BindVertexBuffer(const RHIBuffer& buffer, uint32_t binding)
{
  m_CommandBuffer->BindVertexBuffer(dynamic_cast<const OpenGLBuffer&>(buffer),
                                    binding);
}

void OpenGLDevice::SetVertexInput(const VertexInputLayout& layout)
{
  m_CommandBuffer->SetVertexInput(layout);
}

void OpenGLDevice::SetPrimitiveTopology(PrimitiveTopology topology)
{
  m_CommandBuffer->SetPrimitiveTopology(topology);
}

void OpenGLDevice::Draw(uint32_t vertex_count,
                        uint32_t instance_count,
                        uint32_t first_vertex,
                        uint32_t first_instance)
{
  m_CommandBuffer->Draw(
      vertex_count, instance_count, first_vertex, first_instance);
}

auto OpenGLDevice::CreateDescriptorSetLayout(const DescriptorSetLayoutDesc& desc)
    -> std::shared_ptr<RHIDescriptorSetLayout>
{
  return std::make_shared<OpenGLDescriptorSetLayout>(desc);
}

auto OpenGLDevice::CreateDescriptorSet(
    const std::shared_ptr<RHIDescriptorSetLayout>& layout)
    -> std::unique_ptr<RHIDescriptorSet>
{
  return std::make_unique<OpenGLDescriptorSet>(layout);
}

auto OpenGLDevice::CreatePipelineLayout(const PipelineLayoutDesc& desc)
    -> std::shared_ptr<RHIPipelineLayout>
{
  return std::make_shared<OpenGLPipelineLayout>(desc);
}

void OpenGLDevice::BindDescriptorSet(
    [[maybe_unused]] uint32_t set_index,
    const RHIDescriptorSet& descriptor_set,
    [[maybe_unused]] const RHIPipelineLayout& layout)
{
  // For OpenGL, we directly bind the UBOs to their binding points
  // The set_index is ignored since OpenGL has a flat binding namespace
  const auto& gl_descriptor_set =
      dynamic_cast<const OpenGLDescriptorSet&>(descriptor_set);
  gl_descriptor_set.Bind();
}
