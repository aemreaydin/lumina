#ifndef CORE_WINDOW_HPP
#define CORE_WINDOW_HPP

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <utility>

#include "Renderer/RendererConfig.hpp"

class Event;

constexpr uint32_t WIDTH = 1280;
constexpr uint32_t HEIGHT = 720;

struct WindowDimensions
{
  uint32_t Width {WIDTH};
  uint32_t Height {HEIGHT};
};

struct WindowProps
{
  std::string Title;
  WindowDimensions Dimensions;
  bool VSync;
  RenderAPI API {RenderAPI::OpenGL};
  std::function<void(void*)> EventCallback;

  explicit WindowProps(std::string title = "Lumina Engine",
                       WindowDimensions dimensions = WindowDimensions {},
                       bool v_sync = false,
                       RenderAPI api = RenderAPI::OpenGL)
      : Title(std::move(title))
      , Dimensions(dimensions)
      , VSync(v_sync)
      , API(api)
  {
  }
};

class Window
{
public:
  Window() = default;
  Window(const Window&) = delete;
  Window(Window&&) = delete;
  auto operator=(const Window&) -> Window& = delete;
  auto operator=(Window&&) -> Window& = delete;
  virtual ~Window() = default;

  virtual void Init(WindowProps props) = 0;
  virtual void OnUpdate() = 0;

  [[nodiscard]] virtual auto GetWidth() const -> uint32_t = 0;
  [[nodiscard]] virtual auto GetHeight() const -> uint32_t = 0;

  virtual void SetEventCallback(const std::function<void(void*)>& callback) = 0;
  virtual void SetVSync(bool enabled) = 0;
  [[nodiscard]] virtual auto IsVSync() const -> bool = 0;
  [[nodiscard]] virtual auto ShouldClose() const -> bool = 0;
  virtual void RequestClose() = 0;

  [[nodiscard]] virtual auto GetNativeWindow() const -> void* = 0;
  [[nodiscard]] virtual auto GetDisplayScale() const -> float = 0;

  static auto Create(const WindowProps& props = WindowProps())
      -> std::unique_ptr<Window>;
};

#endif
