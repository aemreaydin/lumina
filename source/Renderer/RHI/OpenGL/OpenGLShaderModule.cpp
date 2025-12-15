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
  if (desc.SPIRVCode.empty()) {
    throw std::runtime_error("Shader SPIR-V code is empty");
  }

  m_Shader = glCreateShader(ShaderStageToGL(m_Stage));
  if (m_Shader == 0) {
    throw std::runtime_error("Failed to create OpenGL shader");
  }

  // Load SPIR-V binary
  glShaderBinary(
      1,
      &m_Shader,
      GL_SHADER_BINARY_FORMAT_SPIR_V,
      desc.SPIRVCode.data(),
      static_cast<GLsizei>(desc.SPIRVCode.size() * sizeof(uint32_t)));

  GLenum err = glGetError();
  if (err != GL_NO_ERROR) {
    glDeleteShader(m_Shader);
    throw std::runtime_error(
        std::format("Failed to load SPIR-V binary: GL error {}", err));
  }

  glSpecializeShader(m_Shader, desc.EntryPoint.c_str(), 0, nullptr, nullptr);

  // Check compilation status
  GLint success = 0;
  glGetShaderiv(m_Shader, GL_COMPILE_STATUS, &success);
  if (success == 0) {
    GLint log_length = 0;
    glGetShaderiv(m_Shader, GL_INFO_LOG_LENGTH, &log_length);

    std::string info_log(static_cast<size_t>(log_length), '\0');
    glGetShaderInfoLog(m_Shader, log_length, nullptr, info_log.data());

    glDeleteShader(m_Shader);
    throw std::runtime_error(
        std::format("Failed to specialize SPIR-V shader: {}", info_log));
  }

  Logger::Trace("[OpenGL] Created {} shader module from SPIR-V",
                ToString(m_Stage));
}

OpenGLShaderModule::~OpenGLShaderModule()
{
  if (m_Shader != 0) {
    glDeleteShader(m_Shader);
  }

  Logger::Trace("[OpenGL] Destroyed shader module");
}
