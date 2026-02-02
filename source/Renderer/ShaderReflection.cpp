#include <format>
#include <stdexcept>

#include "Renderer/ShaderReflection.hpp"

#include "Core/Logger.hpp"
#include "Renderer/RHI/RHIDescriptorSet.hpp"
#include "Renderer/RHI/RHIDevice.hpp"

auto ShaderReflectionData::FindParameterByName(std::string_view name) const
    -> const ShaderParameterInfo&
{
  for (const auto& set : DescriptorSets) {
    for (const auto& param : set.Parameters) {
      if (param.Name == name) {
        return param;
      }
    }
  }

  throw std::runtime_error(
      std::format("Shader parameter '{}' not found", name));
}

auto ShaderReflectionData::FindDescriptorSet(uint32_t set_index) const
    -> const ShaderDescriptorSetInfo&
{
  for (const auto& set : DescriptorSets) {
    if (set.SetIndex == set_index) {
      return set;
    }
  }

  throw std::runtime_error(
      std::format("Descriptor set {} not found", set_index));
}

auto ReflectedPipelineLayout::GetSetLayout(const std::string& parameter_name)
    const -> std::shared_ptr<RHIDescriptorSetLayout>
{
  auto iter = ParameterSetIndex.find(parameter_name);
  if (iter != ParameterSetIndex.end() && iter->second < SetLayouts.size()) {
    return SetLayouts[iter->second];
  }
  return nullptr;
}

auto ReflectedPipelineLayout::GetSetIndex(
    const std::string& parameter_name) const -> uint32_t
{
  auto iter = ParameterSetIndex.find(parameter_name);
  if (iter != ParameterSetIndex.end()) {
    return iter->second;
  }
  throw std::runtime_error(std::format(
      "Parameter '{}' not found in reflected layout", parameter_name));
}

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

auto CreatePipelineLayoutFromReflection(RHIDevice& device,
                                        const ShaderReflectionData& reflection)
    -> ReflectedPipelineLayout
{
  ReflectedPipelineLayout result;

  for (const auto& set : reflection.DescriptorSets) {
    for (const auto& param : set.Parameters) {
      Logger::Info("Param: {}", param.Name);
    }
  }

  for (const auto& set : reflection.DescriptorSets) {
    DescriptorSetLayoutDesc layout_desc;

    for (const auto& param : set.Parameters) {
      DescriptorBinding binding;
      binding.Binding = param.Binding;
      binding.Type = MapToDescriptorType(param.Type);
      binding.Stages = param.Stages;
      binding.Count = param.Count;
      layout_desc.Bindings.push_back(binding);

      result.ParameterSetIndex[param.Name] = set.SetIndex;
    }

    if (!layout_desc.Bindings.empty()) {
      auto layout = device.CreateDescriptorSetLayout(layout_desc);

      if (result.SetLayouts.size() <= set.SetIndex) {
        result.SetLayouts.resize(static_cast<size_t>(set.SetIndex) + 1);
      }
      result.SetLayouts[set.SetIndex] = layout;

      Logger::Info(
          "Auto-generated descriptor set layout for set {} with {} bindings",
          set.SetIndex,
          layout_desc.Bindings.size());
    }
  }

  return result;
}
