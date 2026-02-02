#include "Renderer/ShaderParameterPolicy.hpp"

#include <format>
#include <stdexcept>

#include "Core/Logger.hpp"
#include "Renderer/RHI/RHIDescriptorSet.hpp"
#include "Renderer/RHI/RHIDevice.hpp"

static auto MapToDescriptorType(ShaderParameterType type) -> DescriptorType
{
  switch (type) {
    case ShaderParameterType::UniformBuffer:
      return DescriptorType::UniformBuffer;
    case ShaderParameterType::DynamicUniformBuffer:
      return DescriptorType::DynamicUniformBuffer;
    case ShaderParameterType::CombinedImageSampler:
      return DescriptorType::CombinedImageSampler;
    case ShaderParameterType::SampledImage:
      return DescriptorType::SampledImage;
    case ShaderParameterType::Sampler:
      return DescriptorType::Sampler;
    case ShaderParameterType::StorageBuffer:
      return DescriptorType::StorageBuffer;
    default:
      return DescriptorType::UniformBuffer;
  }
}

auto ReflectedPipelineLayout::GetSetLayout(
    const std::string& parameter_name) const
    -> std::shared_ptr<RHIDescriptorSetLayout>
{
  auto it = ParameterSetIndex.find(parameter_name);
  if (it != ParameterSetIndex.end() && it->second < SetLayouts.size()) {
    return SetLayouts[it->second];
  }
  return nullptr;
}

auto ReflectedPipelineLayout::GetSetIndex(
    const std::string& parameter_name) const -> uint32_t
{
  auto it = ParameterSetIndex.find(parameter_name);
  if (it != ParameterSetIndex.end()) {
    return it->second;
  }
  throw std::runtime_error(
      std::format("Parameter '{}' not found in reflected layout",
                  parameter_name));
}

auto CreatePipelineLayoutFromReflection(
    RHIDevice& device,
    const ShaderReflectionData& reflection,
    const ShaderParameterPolicy& policy) -> ReflectedPipelineLayout
{
  ReflectedPipelineLayout result;

  // Create descriptor set layouts from reflection
  for (const auto& set : reflection.DescriptorSets) {
    DescriptorSetLayoutDesc layout_desc;

    for (const auto& param : set.Parameters) {
      DescriptorBinding binding;
      binding.Binding = param.Binding;
      binding.Type = MapToDescriptorType(param.Type);

      // If the parameter name is in DynamicBufferNames and it's a UBO,
      // use DynamicUniformBuffer instead
      if (binding.Type == DescriptorType::UniformBuffer
          && policy.DynamicBufferNames.contains(param.Name))
      {
        binding.Type = DescriptorType::DynamicUniformBuffer;
      }

      binding.Stages = param.Stages;
      binding.Count = param.Count;
      layout_desc.Bindings.push_back(binding);

      // Record parameter name -> set index mapping
      result.ParameterSetIndex[param.Name] = set.SetIndex;
    }

    if (!layout_desc.Bindings.empty()) {
      auto layout = device.CreateDescriptorSetLayout(layout_desc);
      // Ensure SetLayouts vector is large enough for the set index
      if (result.SetLayouts.size() <= set.SetIndex) {
        result.SetLayouts.resize(set.SetIndex + 1);
      }
      result.SetLayouts[set.SetIndex] = layout;

      Logger::Info("Auto-generated descriptor set layout for set {} with {} "
                   "bindings",
                   set.SetIndex,
                   layout_desc.Bindings.size());
    }
  }

  // Copy non-null set layouts to the pipeline layout desc
  for (const auto& layout : result.SetLayouts) {
    result.Desc.SetLayouts.push_back(layout);
  }

  return result;
}
