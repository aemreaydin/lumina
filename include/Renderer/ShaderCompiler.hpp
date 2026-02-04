#ifndef RENDERER_SHADER_COMPILER_HPP
#define RENDERER_SHADER_COMPILER_HPP

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "Renderer/RendererConfig.hpp"
#include "Renderer/ShaderReflection.hpp"

enum class ShaderType : std::uint8_t
{
  Vertex,
  Fragment,
  Compute
};
using ShaderSources = std::map<ShaderType, std::vector<uint32_t>>;
using ShaderGLSLSources = std::map<ShaderType, std::string>;

struct ShaderCompileResult
{
  ShaderSources Sources;
  ShaderGLSLSources GLSLSources;
  ShaderReflectionData Reflection;

  [[nodiscard]] auto GetSPIRV(ShaderType type) const -> std::vector<uint32_t>
  {
    auto it = Sources.find(type);
    return it != Sources.end() ? it->second : std::vector<uint32_t> {};
  }

  [[nodiscard]] auto GetGLSL(ShaderType type) const -> std::string
  {
    auto it = GLSLSources.find(type);
    return it != GLSLSources.end() ? it->second : "";
  }
};

class ShaderCompiler
{
public:
  static auto Compile(const std::string& shader_path, RenderAPI api)
      -> ShaderCompileResult;
};

#endif
