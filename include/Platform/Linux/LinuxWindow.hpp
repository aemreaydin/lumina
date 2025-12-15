#ifndef PLATFORM_LINUXWINDOW_HPP
#define PLATFORM_LINUXWINDOW_HPP

#include <cstdint>
#include <functional>

#include <SDL3/SDL.h>

#include "Core/Window.hpp"

class LinuxWindow : public Window
{
public:
  LinuxWindow() = default;
  LinuxWindow(const LinuxWindow&) = delete;
  LinuxWindow(LinuxWindow&&) = delete;
  auto operator=(const LinuxWindow&) -> LinuxWindow& = delete;
  auto operator=(LinuxWindow&&) -> LinuxWindow& = delete;
  ~LinuxWindow() override;

  void Init(WindowProps props) override;
  void OnUpdate() override;

  [[nodiscard]] auto GetWidth() const -> uint32_t override
  {
    return m_WindowProps.Dimensions.Width;
  }

  [[nodiscard]] auto GetHeight() const -> uint32_t override
  {
    return m_WindowProps.Dimensions.Height;
  }

  [[nodiscard]] auto GetNativeWindow() const -> void* override
  {
    return m_Window;
  }

  void SetEventCallback(const std::function<void(void*)>& callback) override
  {
    m_WindowProps.EventCallback = callback;
  }

  void SetVSync(bool enabled) override;

  [[nodiscard]] auto IsVSync() const -> bool override
  {
    return m_WindowProps.VSync;
  }

  [[nodiscard]] auto ShouldClose() const -> bool override
  {
    return m_ShouldClose;
  }

private:
  SDL_Window* m_Window {nullptr};
  bool m_ShouldClose {false};

  WindowProps m_WindowProps;
};

#endif
