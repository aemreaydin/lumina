#include <format>
#include <stdexcept>
#include <unordered_map>

#include "Renderer/RHI/OpenGL/OpenGLShaderModule.hpp"

#include "Core/Logger.hpp"

static constexpr uint32_t kBindingStride = 16;

static constexpr uint32_t kSpvOpDecorate = 71;
static constexpr uint32_t kSpvDecorationBinding = 33;
static constexpr uint32_t kSpvDecorationDescriptorSet = 34;

static void FlattenSpirvBindings(std::vector<uint32_t>& spirv)
{
  if (spirv.size() < 5) {
    return;
  }

  struct DecorationInfo
  {
    uint32_t Set {0};
    uint32_t Binding {0};
    size_t SetWordOffset {0};
    size_t BindingWordOffset {0};
    bool HasSet {false};
    bool HasBinding {false};
  };

  std::unordered_map<uint32_t, DecorationInfo> decorations;

  size_t i = 5;
  while (i < spirv.size()) {
    uint32_t word0 = spirv[i];
    auto word_count = static_cast<uint16_t>(word0 >> 16);
    auto opcode = static_cast<uint16_t>(word0 & 0xFFFF);

    if (word_count == 0) {
      break;
    }

    if (opcode == kSpvOpDecorate && word_count >= 4) {
      uint32_t target_id = spirv[i + 1];
      uint32_t decoration = spirv[i + 2];

      if (decoration == kSpvDecorationDescriptorSet) {
        auto& info = decorations[target_id];
        info.Set = spirv[i + 3];
        info.SetWordOffset = i + 3;
        info.HasSet = true;
      } else if (decoration == kSpvDecorationBinding) {
        auto& info = decorations[target_id];
        info.Binding = spirv[i + 3];
        info.BindingWordOffset = i + 3;
        info.HasBinding = true;
      }
    }

    i += word_count;
  }

  for (auto& [id, info] : decorations) {
    if (info.HasSet && info.HasBinding) {
      uint32_t flat_binding = info.Set * kBindingStride + info.Binding;
      spirv[info.SetWordOffset] = 0;
      spirv[info.BindingWordOffset] = flat_binding;
    }
  }
}

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

  auto spirv = desc.SPIRVCode;
  FlattenSpirvBindings(spirv);

  m_Shader = glCreateShader(ShaderStageToGL(m_Stage));
  if (m_Shader == 0) {
    throw std::runtime_error("Failed to create OpenGL shader");
  }

  glShaderBinary(1,
                 &m_Shader,
                 GL_SHADER_BINARY_FORMAT_SPIR_V,
                 spirv.data(),
                 static_cast<GLsizei>(spirv.size() * sizeof(uint32_t)));

  GLenum err = glGetError();
  if (err != GL_NO_ERROR) {
    glDeleteShader(m_Shader);
    throw std::runtime_error(
        std::format("Failed to load SPIR-V binary: GL error {}", err));
  }

  glSpecializeShader(m_Shader, desc.EntryPoint.c_str(), 0, nullptr, nullptr);

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
