#include <format>
#include <stdexcept>

#include "Renderer/RHI/OpenGL/OpenGLShaderModule.hpp"

#include "Core/Logger.hpp"

static auto ShaderStageToGL(ShaderStage stage) -> GLenum
{
  switch (stage) {
    case ShaderStage::Vertex:
      return GL_VERTEX_SHADER;
    case ShaderStage::Fragment:
      return GL_FRAGMENT_SHADER;
    case ShaderStage::Compute:
      return GL_COMPUTE_SHADER;
  }
  return GL_VERTEX_SHADER;
}

OpenGLShaderModule::OpenGLShaderModule(const ShaderModuleDesc& desc)
    : m_Stage(desc.Stage)
{
  if (desc.GLSLCode.empty()) {
    throw std::runtime_error("No GLSL code provided for OpenGL shader");
  }

  m_Shader = glCreateShader(ShaderStageToGL(m_Stage));
  if (m_Shader == 0) {
    throw std::runtime_error("Failed to create OpenGL shader");
  }

  const char* source = desc.GLSLCode.c_str();
  glShaderSource(m_Shader, 1, &source, nullptr);
  glCompileShader(m_Shader);

  GLint success = 0;
  glGetShaderiv(m_Shader, GL_COMPILE_STATUS, &success);
  if (success == GL_FALSE) {
    GLint log_length = 0;
    glGetShaderiv(m_Shader, GL_INFO_LOG_LENGTH, &log_length);

    std::string info_log(static_cast<size_t>(log_length), '\0');
    glGetShaderInfoLog(m_Shader, log_length, nullptr, info_log.data());

    glDeleteShader(m_Shader);
    m_Shader = 0;
    throw std::runtime_error(
        std::format("Failed to compile GLSL shader: {}", info_log));
  }

  Logger::Trace("[OpenGL] Created {} shader module from GLSL",
                ToString(m_Stage));
}

OpenGLShaderModule::~OpenGLShaderModule()
{
  if (m_Shader != 0) {
    glDeleteShader(m_Shader);
  }

  Logger::Trace("[OpenGL] Destroyed shader module");
}
