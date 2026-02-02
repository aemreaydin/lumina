#ifndef RENDERER_SHADER_PARAMETER_POLICY_HPP
#define RENDERER_SHADER_PARAMETER_POLICY_HPP

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "Renderer/RHI/RHIPipeline.hpp"
#include "Renderer/ShaderReflection.hpp"

class RHIDevice;
class RHIDescriptorSetLayout;

struct ShaderParameterPolicy
{
  std::unordered_set<std::string> DynamicBufferNames;
};

struct ReflectedPipelineLayout
{
  PipelineLayoutDesc Desc;
  std::vector<std::shared_ptr<RHIDescriptorSetLayout>> SetLayouts;

  // Maps parameter name -> set index (populated from reflection)
  std::unordered_map<std::string, uint32_t> ParameterSetIndex;

  [[nodiscard]] auto GetSetLayout(const std::string& parameter_name) const
      -> std::shared_ptr<RHIDescriptorSetLayout>;

  [[nodiscard]] auto GetSetIndex(const std::string& parameter_name) const
      -> uint32_t;
};

auto CreatePipelineLayoutFromReflection(
    RHIDevice& device,
    const ShaderReflectionData& reflection,
    const ShaderParameterPolicy& policy) -> ReflectedPipelineLayout;

#endif
