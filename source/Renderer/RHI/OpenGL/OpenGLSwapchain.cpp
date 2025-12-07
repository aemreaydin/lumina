#include "Renderer/RHI/OpenGL/OpenGLSwapchain.hpp"

#include <GLFW/glfw3.h>
#include <glad/glad.h>

OpenGLSwapchain::OpenGLSwapchain(GLFWwindow* window)
    : m_Window(window)
{
  if (m_Window != nullptr) {
    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(m_Window, &width, &height);
    m_Width = static_cast<uint32_t>(width);
    m_Height = static_cast<uint32_t>(height);
  }
}

void OpenGLSwapchain::Resize(uint32_t width, uint32_t height)
{
  m_Width = width;
  m_Height = height;
  glViewport(0, 0, static_cast<int>(width), static_cast<int>(height));
}
