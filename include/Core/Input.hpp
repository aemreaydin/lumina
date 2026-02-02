#ifndef CORE_INPUT_HPP
#define CORE_INPUT_HPP

#include <array>
#include <cstdint>

#include <linalg/vec.hpp>

enum class KeyCode : uint8_t
{
  Unknown = 0,
  A = 4,
  B = 5,
  C = 6,
  D = 7,
  E = 8,
  F = 9,
  G = 10,
  H = 11,
  I = 12,
  J = 13,
  K = 14,
  L = 15,
  M = 16,
  N = 17,
  O = 18,
  P = 19,
  Q = 20,
  R = 21,
  S = 22,
  T = 23,
  U = 24,
  V = 25,
  W = 26,
  X = 27,
  Y = 28,
  Z = 29,
  // Numbers
  Num1 = 30,
  Num2 = 31,
  Num3 = 32,
  Num4 = 33,
  Num5 = 34,
  Num6 = 35,
  Num7 = 36,
  Num8 = 37,
  Num9 = 38,
  Num0 = 39,
  // Special keys
  Return = 40,
  Escape = 41,
  Backspace = 42,
  Tab = 43,
  Space = 44,
  // Modifiers
  LShift = 225,
  RShift = 229,
  LCtrl = 224,
  RCtrl = 228,
  LAlt = 226,
  RAlt = 230,
  // Arrow keys
  Right = 79,
  Left = 80,
  Down = 81,
  Up = 82,
  // Function keys
  F1 = 58,
  F2 = 59,
  F3 = 60,
  F4 = 61,
  F5 = 62,
  F6 = 63,
  F7 = 64,
  F8 = 65,
  F9 = 66,
  F10 = 67,
  F11 = 68,
  F12 = 69,
};

enum class MouseButton : uint8_t
{
  Left = 1,
  Middle = 2,
  Right = 3,
  X1 = 4,
  X2 = 5,
};

class Input
{
public:
  // Call at start of frame before polling events
  static void BeginFrame();

  // Process SDL event (called from Window event handler)
  static void ProcessEvent(void* sdl_event);

  // Keyboard state
  [[nodiscard]] static auto IsKeyDown(KeyCode key) -> bool;
  [[nodiscard]] static auto IsKeyPressed(KeyCode key) -> bool;
  [[nodiscard]] static auto IsKeyReleased(KeyCode key) -> bool;

  // Mouse button state
  [[nodiscard]] static auto IsMouseButtonDown(MouseButton button) -> bool;
  [[nodiscard]] static auto IsMouseButtonPressed(MouseButton button) -> bool;
  [[nodiscard]] static auto IsMouseButtonReleased(MouseButton button) -> bool;

  // Mouse position and movement
  [[nodiscard]] static auto GetMousePosition() -> linalg::Vec2;
  [[nodiscard]] static auto GetMouseDelta() -> linalg::Vec2;
  [[nodiscard]] static auto GetScrollDelta() -> linalg::Vec2;

  // Mouse capture (for FPS camera - hides cursor and captures mouse)
  static void SetMouseCaptured(bool captured);
  [[nodiscard]] static auto IsMouseCaptured() -> bool;

private:
  static constexpr size_t MAX_KEYS = 512;
  static constexpr size_t MAX_MOUSE_BUTTONS = 8;

  static std::array<bool, MAX_KEYS> SKeyState;
  static std::array<bool, MAX_KEYS> SPrevKeyState;

  static std::array<bool, MAX_MOUSE_BUTTONS> SMouseButtonState;
  static std::array<bool, MAX_MOUSE_BUTTONS> SPrevMouseButtonState;

  // Mouse position and deltas
  static linalg::Vec2 SMousePosition;
  static linalg::Vec2 SMouseDelta;
  static linalg::Vec2 SScrollDelta;

  static bool SMouseCaptured;
};

#endif
