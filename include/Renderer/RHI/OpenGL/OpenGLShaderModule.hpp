#ifndef RENDERER_RHI_OPENGL_OPENGLSHADERMODULE_HPP
#define RENDERER_RHI_OPENGL_OPENGLSHADERMODULE_HPP

#include <glad/glad.h>

#include "Renderer/RHI/RHIShaderModule.hpp"

class OpenGLShaderModule final : public RHIShaderModule
{
public:
  explicit OpenGLShaderModule(const ShaderModuleDesc& desc);

  OpenGLShaderModule(const OpenGLShaderModule&) = delete;
  OpenGLShaderModule(OpenGLShaderModule&&) = delete;
  auto operator=(const OpenGLShaderModule&) -> OpenGLShaderModule& = delete;
  auto operator=(OpenGLShaderModule&&) -> OpenGLShaderModule& = delete;
  ~OpenGLShaderModule() override;

  [[nodiscard]] auto GetStage() const -> ShaderStage override
  {
    return m_Stage;
  }

  [[nodiscard]] auto GetGLShader() const -> GLuint { return m_Shader; }

private:
  GLuint m_Shader {0};
  ShaderStage m_Stage {ShaderStage::Vertex};
};

#endif
