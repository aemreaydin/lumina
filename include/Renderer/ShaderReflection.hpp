#ifndef RENDERER_SHADER_REFLECTION_HPP
#define RENDERER_SHADER_REFLECTION_HPP

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "Renderer/RHI/RHIShaderModule.hpp"

enum class ShaderParameterType : uint8_t
{
  UniformBuffer,
  DynamicUniformBuffer,
  CombinedImageSampler,
  SampledImage,
  Sampler,
  StorageBuffer,
};

struct ShaderParameterInfo
{
  std::string Name;
  ShaderParameterType Type {ShaderParameterType::UniformBuffer};
  uint32_t Set {0};
  uint32_t Binding {0};
  uint32_t Size {0};
  uint32_t Count {1};
  ShaderStage Stages {ShaderStage::Vertex};
};

struct ShaderDescriptorSetInfo
{
  uint32_t SetIndex {0};
  std::vector<ShaderParameterInfo> Parameters;
};

struct ShaderReflectionData
{
  std::vector<ShaderDescriptorSetInfo> DescriptorSets;
  std::string SourcePath;

  [[nodiscard]] auto FindParameterByName(std::string_view name) const
      -> const ShaderParameterInfo&;

  [[nodiscard]] auto FindDescriptorSet(uint32_t set_index) const
      -> const ShaderDescriptorSetInfo&;
};

#endif
