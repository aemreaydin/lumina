#ifndef RENDERER_SHADER_COMPILER_HPP
#define RENDERER_SHADER_COMPILER_HPP

#include <cstdint>
#include <map>
#include <string>
#include <vector>

enum class ShaderType : std::uint8_t
{
  Vertex,
  Fragment,
  Compute
};
using ShaderSources = std::map<ShaderType, std::vector<uint32_t>>;

class ShaderCompiler
{
  static auto Compile(const std::string& shader_path) -> ShaderSources;
};

#endif
