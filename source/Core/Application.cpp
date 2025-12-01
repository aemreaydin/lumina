#include "Core/Application.hpp"

#include "Core/Logger.hpp"
#include "GLFW/glfw3.h"

Application::Application()
{
  Logger::Info("Creating application");
  m_Window = Window::Create();
  m_Window->SetEventCallback([this](void* event) -> void
                             { this->OnEvent(event); });
  Logger::Info("Application created successfully");
}

void Application::OnEvent([[maybe_unused]] void* event)
{
  Logger::Trace("Event received: {}", m_Window->GetWidth());
}

void Application::Run()
{
  Logger::Info("Starting application main loop");

  while (m_Running) {
    const auto current_frame_time = static_cast<float>(glfwGetTime());
    // const auto DeltaTime        = CurrentFrameTime - m_LastFrameTime;
    m_LastFrameTime = current_frame_time;

    m_Window->OnUpdate();

    auto* native_win = static_cast<GLFWwindow*>(m_Window->GetNativeWindow());
    if (glfwWindowShouldClose(native_win) != 0) {
      Logger::Info("Main loop exiting");
      m_Running = false;
    }
  }
}
