#include <stdexcept>

#include "Renderer/OpenGLContext.hpp"

#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include "Core/Logger.hpp"

constexpr int MAJOR_VER = 4;
constexpr int MINOR_VER = 6;

static void GLFWErrorCallback(int error, const char* description)
{
  Logger::Error("GLFW Error ({}): {}", error, description);
}

OpenGLContext::~OpenGLContext()
{
  Logger::Trace("OpenGLContext shutting down");
  glfwTerminate();
}

void OpenGLContext::SetGLFWWindow(GLFWwindow* window)
{
  m_Window = window;
  glfwMakeContextCurrent(m_Window);
  Logger::Trace("GLFW window set for OpenGL context");
}

void OpenGLContext::Init()
{
  Logger::Info("Initializing OpenGL context");

  if (glfwInit() == GLFW_FALSE) {
    Logger::Critical("Failed to initialize GLFW");
    throw std::runtime_error("glfwInit failed.");
  }

  glfwSetErrorCallback(GLFWErrorCallback);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, MAJOR_VER);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, MINOR_VER);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  Logger::Info("OpenGL context initialized");
}

void OpenGLContext::LoadFns()
{
  if (gladLoadGL() == 0) {
    Logger::Critical("Failed to load proc address");
    throw std::runtime_error("gladLoadGLLoader failed.");
  }
}

void OpenGLContext::SwapBuffers()
{
  glfwSwapBuffers(m_Window);
}
