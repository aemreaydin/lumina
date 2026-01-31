#include "Renderer/RHI/OpenGL/OpenGLImGui.hpp"

#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl3.h>

#include "Core/Logger.hpp"
#include "Core/Window.hpp"
#include "Renderer/RHI/OpenGL/OpenGLDevice.hpp"
#include "UI/ImGuiStyle.hpp"

OpenGLImGui::OpenGLImGui(OpenGLDevice& device)
    : m_Device(device)
{
}

void OpenGLImGui::Init(Window& window)
{
  Logger::Info("Initializing OpenGL ImGui backend");

  // Setup ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();

  ImGuiIO& imgui_io = ImGui::GetIO();
  imgui_io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

  // Load custom font with DPI scaling
  constexpr float BASE_FONT_SIZE = 15.0F;
  const float scale = window.GetDisplayScale();
  imgui_io.Fonts->AddFontFromFileTTF("fonts/InterVariable.ttf",
                                      BASE_FONT_SIZE * scale);

  // Apply flat theme and scale for HiDPI
  UIStyle::ApplyFlatTheme();
  ImGui::GetStyle().ScaleAllSizes(scale);

  // Initialize SDL3 backend
  auto* sdl_window = static_cast<SDL_Window*>(window.GetNativeWindow());
  ImGui_ImplSDL3_InitForOpenGL(sdl_window, SDL_GL_GetCurrentContext());

  // Initialize OpenGL3 backend
  // Using GLSL version 450 for modern OpenGL
  ImGui_ImplOpenGL3_Init("#version 450");

  Logger::Info("OpenGL ImGui backend initialized");
}

void OpenGLImGui::Shutdown()
{
  Logger::Info("Shutting down OpenGL ImGui backend");

  m_Device.WaitIdle();

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplSDL3_Shutdown();
  ImGui::DestroyContext();
}

void OpenGLImGui::BeginFrame()
{
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplSDL3_NewFrame();
  ImGui::NewFrame();
}

void OpenGLImGui::EndFrame()
{
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
