#include <stdexcept>

#include "Core/Application.hpp"

#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include "Core/ConfigLoader.hpp"
#include "Core/Logger.hpp"
#include "Renderer/RHI/RHIDevice.hpp"

constexpr int OPENGL_MAJOR_VER = 4;
constexpr int OPENGL_MINOR_VER = 6;

static void GLFWErrorCallback(int error, const char* description)
{
  Logger::Error("GLFW Error ({}): {}", error, description);
}

Application::Application()
{
  Logger::Init(ConfigLoader::LoadLoggerConfig("config.toml"));
  Logger::Info("Creating application");

  m_RendererConfig = ConfigLoader::LoadRendererConfig("config.toml");

  init_glfw();

  m_Window = Window::Create();
  m_Window->SetEventCallback([this](void* event) -> void
                             { this->OnEvent(event); });

  m_RHIDevice = RHIDevice::Create(m_RendererConfig);

  m_RHIDevice->Init(m_RendererConfig, m_Window->GetNativeWindow());
  m_RHIDevice->CreateSwapchain(m_Window->GetWidth(), m_Window->GetHeight());

  Logger::Info("Application created successfully");
}

Application::~Application()
{
  Logger::Info("Shutting down application");

  if (m_RHIDevice) {
    m_RHIDevice->Destroy();
  }

  // Destroy window before terminating GLFW
  m_Window.reset();

  glfwTerminate();
  Logger::Info("Application shutdown complete");
}

void Application::init_glfw() const
{
  Logger::Info("Initializing GLFW");

  if (glfwInit() == GLFW_FALSE) {
    Logger::Critical("Failed to initialize GLFW");
    throw std::runtime_error("glfwInit failed.");
  }

  glfwSetErrorCallback(GLFWErrorCallback);

  if (m_RendererConfig.API == RenderAPI::OpenGL) {
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, OPENGL_MAJOR_VER);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, OPENGL_MINOR_VER);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    Logger::Info(
        "GLFW configured for OpenGL {}.{}", OPENGL_MAJOR_VER, OPENGL_MINOR_VER);
  } else if (m_RendererConfig.API == RenderAPI::Vulkan) {
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    Logger::Info("GLFW configured for Vulkan");
  }

  Logger::Info("GLFW initialized successfully");
}

void Application::OnEvent([[maybe_unused]] void* event)
{
  Logger::Info("Event received: {}", m_Window->GetWidth());
}

void Application::Run()
{
  Logger::Info("Starting application main loop");

  while (m_Running) {
    const auto current_frame_time = static_cast<float>(glfwGetTime());
    const auto delta_time = current_frame_time - m_LastFrameTime;
    m_LastFrameTime = current_frame_time;
    Logger::Trace("delta_time: {}", delta_time);

    // Poll events
    m_Window->OnUpdate();

    // Begin rendering
    m_RHIDevice->BeginFrame();

    // Rendering commands go here (currently just clears to red via BeginFrame)

    // End rendering
    m_RHIDevice->EndFrame();

    // Present to screen
    m_RHIDevice->Present();

    auto* native_win = static_cast<GLFWwindow*>(m_Window->GetNativeWindow());
    if (glfwWindowShouldClose(native_win) != 0) {
      Logger::Info("Main loop exiting");
      m_Running = false;
    }
  }
}
