#include <stdexcept>

#include "Core/Application.hpp"

#include <SDL3/SDL.h>

#include "Core/ConfigLoader.hpp"
#include "Core/Logger.hpp"
#include "Renderer/RHI/RHIDevice.hpp"

void Application::Init()
{
  Logger::Init(ConfigLoader::LoadLoggerConfig("config.toml"));
  Logger::Info("Creating application");

  m_RendererConfig = ConfigLoader::LoadRendererConfig("config.toml");

  InitSdl();

  // Create window with render API info
  WindowProps props;
  props.API = m_RendererConfig.API;
  m_Window = Window::Create(props);
  m_Window->SetEventCallback([this](void* event) -> void
                             { this->OnEvent(event); });

  m_RHIDevice = RHIDevice::Create(m_RendererConfig);

  m_RHIDevice->Init(m_RendererConfig, m_Window->GetNativeWindow());
  m_RHIDevice->CreateSwapchain(m_Window->GetWidth(), m_Window->GetHeight());

  // Initialize timing
  m_StartTime = SDL_GetPerformanceCounter();
  m_LastFrameTime = m_StartTime;

  OnInit();

  Logger::Info("Application created successfully");
}

void Application::Destroy()
{
  Logger::Info("Shutting down application");

  if (m_RHIDevice) {
    m_RHIDevice->WaitIdle();
  }

  OnDestroy();

  if (m_RHIDevice) {
    m_RHIDevice->Destroy();
  }

  // Destroy window before quitting SDL
  m_Window.reset();

  SDL_Quit();
  Logger::Info("Application shutdown complete");
}

void Application::InitSdl()
{
  Logger::Info("Initializing SDL3");

  if (!SDL_Init(SDL_INIT_VIDEO)) {
    Logger::Critical("Failed to initialize SDL: {}", SDL_GetError());
    throw std::runtime_error("SDL_Init failed.");
  }

  Logger::Info("SDL3 initialized successfully");
}

void Application::OnEvent([[maybe_unused]] void* event)
{
  Logger::Info("Event received: {}", m_Window->GetWidth());
}

void Application::Run()
{
  Logger::Info("Starting application main loop");

  while (m_Running) {
    // Calculate delta time using SDL high-resolution timer
    const auto current_time = SDL_GetPerformanceCounter();
    const auto frequency = SDL_GetPerformanceFrequency();
    const auto delta_time = static_cast<float>(current_time - m_LastFrameTime)
        / static_cast<float>(frequency);
    m_LastFrameTime = current_time;

    // Poll events
    m_Window->OnUpdate();

    // Check for window close
    if (m_Window->ShouldClose()) {
      Logger::Info("Main loop exiting");
      m_Running = false;
      continue;
    }

    // Begin rendering
    m_RHIDevice->BeginFrame();

    OnRender(delta_time);

    // End rendering
    m_RHIDevice->EndFrame();

    // Present to screen
    m_RHIDevice->Present();
  }
}
