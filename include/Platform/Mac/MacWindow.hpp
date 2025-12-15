#ifndef PLATFORM_MACWINDOW_HPP
#define PLATFORM_MACWINDOW_HPP

#include <cstdint>
#include <functional>

#include <SDL3/SDL.h>

#include "Core/Window.hpp"

class MacWindow : public Window
{
public:
  MacWindow() = default;
  MacWindow(const MacWindow&) = delete;
  MacWindow(MacWindow&&) = delete;
  auto operator=(const MacWindow&) -> MacWindow& = delete;
  auto operator=(MacWindow&&) -> MacWindow& = delete;
  ~MacWindow() override;

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
