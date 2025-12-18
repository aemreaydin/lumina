#include "Platform/Mac/MacWindow.hpp"

#include "Core/Logger.hpp"

MacWindow::~MacWindow()
{
  Logger::Trace("Destroying Mac window");
  if (m_Window != nullptr) {
    SDL_DestroyWindow(m_Window);
  }
}

void MacWindow::Init(WindowProps props)
{
  m_WindowProps = props;
  Logger::Info("Creating Mac window: {} ({}x{})",
               m_WindowProps.Title,
               m_WindowProps.Dimensions.Width,
               m_WindowProps.Dimensions.Height);

  // Determine SDL window flags based on render API
  SDL_WindowFlags flags = SDL_WINDOW_RESIZABLE;
  if (m_WindowProps.API == RenderAPI::OpenGL) {
    flags |= SDL_WINDOW_OPENGL;
  } else if (m_WindowProps.API == RenderAPI::Vulkan) {
    flags |= SDL_WINDOW_VULKAN;
  }

  m_Window = SDL_CreateWindow(m_WindowProps.Title.c_str(),
                              static_cast<int>(m_WindowProps.Dimensions.Width),
                              static_cast<int>(m_WindowProps.Dimensions.Height),
                              flags);

  if (m_Window == nullptr) {
    Logger::Critical("Failed to create SDL window: {}", SDL_GetError());
    throw std::runtime_error("SDL_CreateWindow failed.");
  }

  // Store WindowProps pointer for event handling
  SDL_SetPointerProperty(
      SDL_GetWindowProperties(m_Window), "props", &m_WindowProps);

  Logger::Info("Mac window created successfully");
}

void MacWindow::SetVSync(bool enabled)
{
  // VSync is handled by OpenGL context (SDL_GL_SetSwapInterval)
  // This will be called after context creation by OpenGLDevice
  m_WindowProps.VSync = enabled;
  Logger::Info("VSync {}", enabled ? "enabled" : "disabled");
}

void MacWindow::OnUpdate()
{
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    switch (event.type) {
      case SDL_EVENT_QUIT:
        Logger::Info("Window close requested");
        m_ShouldClose = true;
        break;

      case SDL_EVENT_WINDOW_RESIZED:
        m_WindowProps.Dimensions.Width =
            static_cast<uint32_t>(event.window.data1);
        m_WindowProps.Dimensions.Height =
            static_cast<uint32_t>(event.window.data2);
        Logger::Trace("Window resized: {}x{}",
                      m_WindowProps.Dimensions.Width,
                      m_WindowProps.Dimensions.Height);
        if (m_WindowProps.EventCallback) {
          m_WindowProps.EventCallback(&event);
        }
        break;

      case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        Logger::Info("Window close requested");
        m_ShouldClose = true;
        break;

      case SDL_EVENT_KEY_DOWN:
      case SDL_EVENT_KEY_UP:
        if (m_WindowProps.EventCallback) {
          m_WindowProps.EventCallback(&event);
        }
        break;

      default:
        break;
    }
  }
}
