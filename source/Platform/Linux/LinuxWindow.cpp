#include <cassert>

#include "Platform/Linux/LinuxWindow.hpp"

#include "Core/Logger.hpp"
#include "GLFW/glfw3.h"
#include "Renderer/OpenGLContext.hpp"

LinuxWindow::~LinuxWindow()
{
  Logger::Trace("Destroying Linux window");
  glfwDestroyWindow(m_Window);
}

void LinuxWindow::Init(WindowProps props)
{
  m_WindowProps = props;
  Logger::Info("Creating Linux window: {} ({}x{})",
               m_WindowProps.Title,
               m_WindowProps.Dimensions.Width,
               m_WindowProps.Dimensions.Height);

  m_GraphicsContext = std::make_unique<OpenGLContext>();
  m_GraphicsContext->Init();
  m_Window = glfwCreateWindow(static_cast<int>(m_WindowProps.Dimensions.Width),
                              static_cast<int>(m_WindowProps.Dimensions.Height),
                              m_WindowProps.Title.c_str(),
                              nullptr,
                              nullptr);

  if (m_Window == nullptr) {
    const char* error {nullptr};
    glfwGetError(&error);
    Logger::Critical("Failed to create GLFW window: {}", error);
    throw std::runtime_error("glfwCreateWindow failed.");
  }

  m_GraphicsContext->SetGLFWWindow(m_Window);
  glfwSetWindowUserPointer(m_Window, &m_WindowProps);
  m_GraphicsContext->LoadFns();
  SetVSync(/*enabled=*/true);

  glfwSetWindowSizeCallback(
      m_Window,
      [](GLFWwindow* window, int width, int height) -> void
      {
        WindowProps& wnd_props =
            *static_cast<WindowProps*>(glfwGetWindowUserPointer(window));
        wnd_props.Dimensions.Width = static_cast<uint32_t>(width);
        wnd_props.Dimensions.Height = static_cast<uint32_t>(height);
        Logger::Trace("Window resized: {}x{}", width, height);
        // In the future: Dispatch WindowResizeEvent
      });

  glfwSetWindowCloseCallback(m_Window,
                             []([[maybe_unused]] GLFWwindow* window) -> void
                             {
                               Logger::Info("Window close requested");
                               // In the future: Dispatch WindowCloseEvent
                             });

  glfwSetKeyCallback(
      m_Window,
      [](GLFWwindow* window,
         [[maybe_unused]] int key,
         [[maybe_unused]] int scancode,
         [[maybe_unused]] int action,
         [[maybe_unused]] int mods) -> void
      {
        WindowProps const& wnd_props =
            *static_cast<WindowProps*>(glfwGetWindowUserPointer(window));
        if (wnd_props.EventCallback) {
          wnd_props.EventCallback(nullptr);
        }
      });

  Logger::Info("Linux window created successfully");
}

void LinuxWindow::SetVSync(bool enabled)
{
  if (enabled) {
    glfwSwapInterval(1);
  } else {
    glfwSwapInterval(0);
  }
  m_WindowProps.VSync = enabled;
  Logger::Info("VSync {}", enabled ? "enabled" : "disabled");
}

void LinuxWindow::OnUpdate()
{
  glfwPollEvents();
  m_GraphicsContext->SwapBuffers();
}
