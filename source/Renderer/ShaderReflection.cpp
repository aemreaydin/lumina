#include "Renderer/ShaderReflection.hpp"

#include <format>
#include <stdexcept>

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
