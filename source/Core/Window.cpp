#include "Core/Window.hpp"

#include "Core/Logger.hpp"

#ifdef _WIN32
#  include "Platform/Windows/WindowsWindow.hpp"
#elifdef __linux__
#  include "Platform/Linux/LinuxWindow.hpp"
#endif

auto Window::Create(const WindowProps& props) -> std::unique_ptr<Window>
{
#ifdef _WIN32
  Logger::Trace("Creating Windows window");
  auto window = std::make_unique<WindowsWindow>();
#elifdef __linux__
  Logger::Trace("Creating Linux window");
  auto window = std::make_unique<LinuxWindow>();
#else
  Logger::Error("Unsupported platform for window creation");
  return nullptr;
#endif
  window->Init(props);
  return window;
}
