#include <stdexcept>

#include "Core/Application.hpp"

#include <SDL3/SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>

#include "Core/ConfigLoader.hpp"
#include "Core/Input.hpp"
#include "Core/Logger.hpp"
#include "Renderer/RHI/RHIDevice.hpp"
#include "UI/RHIImGui.hpp"

Application::Application() = default;

Application::~Application() = default;

void Application::Init()
{
  Logger::Init(ConfigLoader::LoadLoggerConfig("config.toml"));
  Logger::Info("Creating application");

  m_RendererConfig = ConfigLoader::LoadRendererConfig("config.toml");

  InitSdl();

  WindowProps props;
  props.API = m_RendererConfig.API;
  m_Window = Window::Create(props);
  m_Window->SetEventCallback([](void* event) -> void
                             { Application::OnEvent(event); });

  m_RHIDevice = RHIDevice::Create(m_RendererConfig);

  m_RHIDevice->Init(m_RendererConfig, m_Window->GetNativeWindow());
  m_RHIDevice->CreateSwapchain(m_Window->GetWidth(), m_Window->GetHeight());

  // Initialize ImGui
  m_ImGui = RHIImGui::Create(*m_RHIDevice);
  m_ImGui->Init(*m_Window);
  m_ImGui->SetCurrentAPI(m_RendererConfig.API);

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

  if (m_ImGui) {
    m_ImGui->Shutdown();
    m_ImGui.reset();
  }

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

void Application::OnEvent(void* event)
{
  auto* sdl_event = static_cast<const SDL_Event*>(event);
  ImGui_ImplSDL3_ProcessEvent(sdl_event);

  const ImGuiIO& imgui_io = ImGui::GetIO();

  // Skip mouse events when ImGui wants the mouse (hovering over a panel)
  if (imgui_io.WantCaptureMouse) {
    switch (sdl_event->type) {
      case SDL_EVENT_MOUSE_MOTION:
      case SDL_EVENT_MOUSE_BUTTON_DOWN:
      case SDL_EVENT_MOUSE_BUTTON_UP:
      case SDL_EVENT_MOUSE_WHEEL:
        return;
      default:
        break;
    }
  }

  // Skip keyboard events when ImGui wants the keyboard (typing in a widget)
  if (imgui_io.WantCaptureKeyboard) {
    switch (sdl_event->type) {
      case SDL_EVENT_KEY_DOWN:
      case SDL_EVENT_KEY_UP:
        return;
      default:
        break;
    }
  }

  Input::ProcessEvent(event);
}

void Application::SwitchBackend(RenderAPI new_api)
{
  Logger::Info("Switching backend to {}",
               new_api == RenderAPI::Vulkan ? "Vulkan" : "OpenGL");

  // Save window dimensions
  const uint32_t width = m_Window->GetWidth();
  const uint32_t height = m_Window->GetHeight();

  m_RendererConfig.API = new_api;

  // Tear down in reverse order
  m_RHIDevice->WaitIdle();
  OnDestroy();

  m_ImGui->Shutdown();
  m_ImGui.reset();

  m_RHIDevice->Destroy();
  m_RHIDevice.reset();

  m_Window.reset();

  // Rebuild with new API
  WindowProps props;
  props.API = new_api;
  props.Dimensions.Width = width;
  props.Dimensions.Height = height;
  m_Window = Window::Create(props);
  m_Window->SetEventCallback([](void* event) -> void
                             { Application::OnEvent(event); });

  m_RHIDevice = RHIDevice::Create(m_RendererConfig);
  m_RHIDevice->Init(m_RendererConfig, m_Window->GetNativeWindow());
  m_RHIDevice->CreateSwapchain(m_Window->GetWidth(), m_Window->GetHeight());

  m_ImGui = RHIImGui::Create(*m_RHIDevice);
  m_ImGui->Init(*m_Window);
  m_ImGui->SetCurrentAPI(new_api);

  OnInit();

  Logger::Info("Backend switch complete");
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

    // Update performance stats
    m_PerfTracker.Update(delta_time);
    m_ImGui->UpdateStats(m_PerfTracker.GetStats());

    // Reset input state for new frame
    Input::BeginFrame();

    // Poll events (forwards to Input::ProcessEvent via OnEvent callback)
    m_Window->OnUpdate();

    // Check for window close
    if (m_Window->ShouldClose()) {
      Logger::Info("Main loop exiting");
      m_Running = false;
      continue;
    }

    // Check for pending backend switch
    if (auto pending = m_ImGui->GetPendingBackendSwitch()) {
      SwitchBackend(*pending);
      m_LastFrameTime = SDL_GetPerformanceCounter();
      continue;
    }

    // Update game logic
    OnUpdate(delta_time);

    // Begin rendering
    m_RHIDevice->BeginFrame();

    // Begin ImGui frame
    m_ImGui->BeginFrame();

    OnRender(delta_time);

    // End ImGui frame (renders draw data)
    m_ImGui->EndFrame();

    // End rendering
    m_RHIDevice->EndFrame();

    // Present to screen
    m_RHIDevice->Present();
  }
}
