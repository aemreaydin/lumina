#include <stdexcept>

#include "Core/Application.hpp"

#include <SDL3/SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>

#include "Core/ConfigLoader.hpp"
#include "Core/Input.hpp"
#include "Core/Logger.hpp"
#include "Renderer/RHI/RHICommandBuffer.hpp"
#include "Renderer/RHI/RHIDevice.hpp"
#include "Renderer/RHI/RHISwapchain.hpp"
#include "Renderer/RenderGraph.hpp"
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

  m_ImGui = RHIImGui::Create(*m_RHIDevice);
  m_ImGui->Init(*m_Window);
  m_ImGui->SetCurrentAPI(m_RendererConfig.API);
  m_ImGui->SetValidationEnabled(m_RendererConfig.EnableValidation);
  m_ImGui->SetResolution(m_Window->GetWidth(), m_Window->GetHeight());

  m_RenderGraph = std::make_unique<RenderGraph>();

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

  m_RenderGraph.reset();

  if (m_ImGui) {
    m_ImGui->Shutdown();
    m_ImGui.reset();
  }

  if (m_RHIDevice) {
    m_RHIDevice->Destroy();
  }

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

  const uint32_t width = m_Window->GetWidth();
  const uint32_t height = m_Window->GetHeight();

  m_RendererConfig.API = new_api;

  m_RHIDevice->WaitIdle();
  m_RenderGraph->Clear();
  OnDestroy();

  m_ImGui->Shutdown();
  m_ImGui.reset();

  m_RHIDevice->Destroy();
  m_RHIDevice.reset();

  m_Window.reset();

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

  m_RenderGraph = std::make_unique<RenderGraph>();
  OnInit();

  Logger::Info("Backend switch complete");
}

auto Application::buildSwapchainPassInfo() -> RenderPassInfo
{
  m_DefaultDepthStencil.DepthLoadOp = LoadOp::Clear;
  m_DefaultDepthStencil.DepthStoreOp = StoreOp::DontCare;
  m_DefaultDepthStencil.ClearDepthStencil.Depth = 1.0F;

  RenderPassInfo info {};
  info.ColorAttachment.ColorLoadOp = LoadOp::Clear;
  info.ColorAttachment.ClearColor = {
      .R = 0.1F, .G = 0.1F, .B = 0.1F, .A = 1.0F};
  if (m_RendererConfig.EnableDepth) {
    info.DepthStencilAttachment = &m_DefaultDepthStencil;
  }
  info.Width = m_RHIDevice->GetSwapchain()->GetWidth();
  info.Height = m_RHIDevice->GetSwapchain()->GetHeight();
  return info;
}

void Application::Run()
{
  Logger::Info("Starting application main loop");

  while (m_Running) {
    const auto current_time = SDL_GetPerformanceCounter();
    const auto frequency = SDL_GetPerformanceFrequency();
    const auto delta_time = static_cast<float>(current_time - m_LastFrameTime)
        / static_cast<float>(frequency);
    m_LastFrameTime = current_time;

    m_PerfTracker.Update(delta_time);
    m_ImGui->UpdateStats(m_PerfTracker.GetStats());

    Input::BeginFrame();

    m_Window->OnUpdate();

    if (m_Window->ShouldClose()) {
      Logger::Info("Main loop exiting");
      m_Running = false;
      continue;
    }

    if (auto pending = m_ImGui->GetPendingBackendSwitch()) {
      SwitchBackend(*pending);
      m_LastFrameTime = SDL_GetPerformanceCounter();
      continue;
    }

    m_ImGui->SetResolution(m_Window->GetWidth(), m_Window->GetHeight());

    OnUpdate(delta_time);

    m_RHIDevice->BeginFrame();
    m_ImGui->BeginFrame();

    auto* cmd = m_RHIDevice->GetCurrentCommandBuffer();

    if (m_RenderGraph->IsCompiled()) {
      auto* swapchain = m_RHIDevice->GetSwapchain();
      m_RenderGraph->SetBackbufferSize(
          swapchain->GetWidth(), swapchain->GetHeight());
      m_RenderGraph->Execute(*cmd);
    } else {
      cmd->BeginRenderPass(buildSwapchainPassInfo());
      OnRender(delta_time);
      m_ImGui->EndFrame();
      cmd->EndRenderPass();
    }

    m_RHIDevice->EndFrame();
    m_RHIDevice->Present();
  }
}
