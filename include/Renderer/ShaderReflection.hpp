#ifndef RENDERER_SHADER_REFLECTION_HPP
#define RENDERER_SHADER_REFLECTION_HPP

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "Renderer/RHI/RHIShaderModule.hpp"

class RHIDevice;

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
  std::string BlockName;
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

struct ReflectedPipelineLayout
{
  std::vector<std::shared_ptr<RHIDescriptorSetLayout>> SetLayouts;
  std::unordered_map<std::string, uint32_t> ParameterSetIndex;

  [[nodiscard]] auto GetSetLayout(const std::string& parameter_name) const
      -> std::shared_ptr<RHIDescriptorSetLayout>;
  [[nodiscard]] auto GetSetIndex(const std::string& parameter_name) const
      -> uint32_t;
};

auto CreatePipelineLayoutFromReflection(RHIDevice& device,
                                        const ShaderReflectionData& reflection)
    -> ReflectedPipelineLayout;

#endif
