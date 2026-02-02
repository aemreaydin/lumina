#ifndef RENDERER_SHADER_COMPILER_HPP
#define RENDERER_SHADER_COMPILER_HPP

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "Renderer/ShaderReflection.hpp"

enum class ShaderType : std::uint8_t
{
  Vertex,
  Fragment,
  Compute
};
using ShaderSources = std::map<ShaderType, std::vector<uint32_t>>;

struct ShaderCompileResult
{
  ShaderSources Sources;
  ShaderReflectionData Reflection;
};

class ShaderCompiler
{
public:
  static auto Compile(const std::string& shader_path) -> ShaderCompileResult;
};

#endif
