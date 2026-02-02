#include "Core/Input.hpp"

#include <SDL3/SDL.h>

// Static member definitions
std::array<bool, Input::MAX_KEYS> Input::SKeyState = {};
std::array<bool, Input::MAX_KEYS> Input::SPrevKeyState = {};
std::array<bool, Input::MAX_MOUSE_BUTTONS> Input::SMouseButtonState = {};
std::array<bool, Input::MAX_MOUSE_BUTTONS> Input::SPrevMouseButtonState = {};
linalg::Vec2 Input::SMousePosition = {};
linalg::Vec2 Input::SMouseDelta = {};
linalg::Vec2 Input::SScrollDelta = {};
bool Input::SMouseCaptured = false;

void Input::BeginFrame()
{
  SPrevKeyState = SKeyState;
  SPrevMouseButtonState = SMouseButtonState;

  SMouseDelta = {0.0F, 0.0F};
  SScrollDelta = {0.0F, 0.0F};
}

void Input::ProcessEvent(void* sdl_event)
{
  auto* event = static_cast<SDL_Event*>(sdl_event);

  switch (event->type) {
    case SDL_EVENT_KEY_DOWN: {
      const auto scancode = static_cast<size_t>(event->key.scancode);
      if (scancode < MAX_KEYS) {
        SKeyState[scancode] = true;
      }
      break;
    }

    case SDL_EVENT_KEY_UP: {
      const auto scancode = static_cast<size_t>(event->key.scancode);
      if (scancode < MAX_KEYS) {
        SKeyState[scancode] = false;
      }
      break;
    }

    case SDL_EVENT_MOUSE_BUTTON_DOWN: {
      const auto button = static_cast<size_t>(event->button.button);
      if (button < MAX_MOUSE_BUTTONS) {
        SMouseButtonState[button] = true;
      }
      break;
    }

    case SDL_EVENT_MOUSE_BUTTON_UP: {
      const auto button = static_cast<size_t>(event->button.button);
      if (button < MAX_MOUSE_BUTTONS) {
        SMouseButtonState[button] = false;
      }
      break;
    }

    case SDL_EVENT_MOUSE_MOTION: {
      SMousePosition = {event->motion.x, event->motion.y};
      SMouseDelta.x() += event->motion.xrel;
      SMouseDelta.y() += event->motion.yrel;
      break;
    }

    case SDL_EVENT_MOUSE_WHEEL: {
      SScrollDelta.x() += event->wheel.x;
      SScrollDelta.y() += event->wheel.y;
      break;
    }

    default:
      break;
  }
}

auto Input::IsKeyDown(KeyCode key) -> bool
{
  const auto scancode = static_cast<size_t>(key);
  if (scancode >= MAX_KEYS) {
    return false;
  }
  return SKeyState[scancode];
}

auto Input::IsKeyPressed(KeyCode key) -> bool
{
  const auto scancode = static_cast<size_t>(key);
  if (scancode >= MAX_KEYS) {
    return false;
  }
  return SKeyState[scancode] && !SPrevKeyState[scancode];
}

auto Input::IsKeyReleased(KeyCode key) -> bool
{
  const auto scancode = static_cast<size_t>(key);
  if (scancode >= MAX_KEYS) {
    return false;
  }
  return !SKeyState[scancode] && SPrevKeyState[scancode];
}

auto Input::IsMouseButtonDown(MouseButton button) -> bool
{
  const auto index = static_cast<size_t>(button);
  if (index >= MAX_MOUSE_BUTTONS) {
    return false;
  }
  return SMouseButtonState[index];
}

auto Input::IsMouseButtonPressed(MouseButton button) -> bool
{
  const auto index = static_cast<size_t>(button);
  if (index >= MAX_MOUSE_BUTTONS) {
    return false;
  }
  return SMouseButtonState[index] && !SPrevMouseButtonState[index];
}

auto Input::IsMouseButtonReleased(MouseButton button) -> bool
{
  const auto index = static_cast<size_t>(button);
  if (index >= MAX_MOUSE_BUTTONS) {
    return false;
  }
  return !SMouseButtonState[index] && SPrevMouseButtonState[index];
}

auto Input::GetMousePosition() -> linalg::Vec2
{
  return SMousePosition;
}

auto Input::GetMouseDelta() -> linalg::Vec2
{
  return SMouseDelta;
}

auto Input::GetScrollDelta() -> linalg::Vec2
{
  return SScrollDelta;
}

void Input::SetMouseCaptured(bool captured)
{
  SMouseCaptured = captured;
  auto* window = SDL_GetKeyboardFocus();
  if (window != nullptr) {
    SDL_SetWindowRelativeMouseMode(window, captured);
  }
}

auto Input::IsMouseCaptured() -> bool
{
  return SMouseCaptured;
}
