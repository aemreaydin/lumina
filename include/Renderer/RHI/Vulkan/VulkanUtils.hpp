#ifndef RENDERER_RHI_VULKAN_VULKANUTILS_HPP
#define RENDERER_RHI_VULKAN_VULKANUTILS_HPP

#include <algorithm>
#include <expected>
#include <span>
#include <string_view>

#include <volk.h>

#include "Renderer/RHI/RHIDescriptorSet.hpp"
#include "Renderer/RHI/RHIShaderModule.hpp"

namespace VkUtils
{

constexpr auto ToVkShaderStage(ShaderStage stage) -> VkShaderStageFlagBits
{
  switch (stage) {
    case ShaderStage::Vertex:
      return VK_SHADER_STAGE_VERTEX_BIT;
    case ShaderStage::Fragment:
      return VK_SHADER_STAGE_FRAGMENT_BIT;
    case ShaderStage::Compute:
      return VK_SHADER_STAGE_COMPUTE_BIT;
  }
  return VK_SHADER_STAGE_VERTEX_BIT;
}

constexpr auto ToVkShaderStageFlags(ShaderStage stage) -> VkShaderStageFlags
{
  return static_cast<VkShaderStageFlags>(ToVkShaderStage(stage));
}

constexpr auto GetNextShaderStage(ShaderStage stage) -> VkShaderStageFlags
{
  switch (stage) {
    case ShaderStage::Vertex:
      return VK_SHADER_STAGE_FRAGMENT_BIT;
    case ShaderStage::Fragment:
    case ShaderStage::Compute:
      return 0;
  }
  return 0;
}

constexpr auto ToVkDescriptorType(DescriptorType type) -> VkDescriptorType
{
  switch (type) {
    case DescriptorType::UniformBuffer:
      return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    case DescriptorType::StorageBuffer:
      return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    case DescriptorType::Sampler:
      return VK_DESCRIPTOR_TYPE_SAMPLER;
    case DescriptorType::SampledImage:
      return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    case DescriptorType::CombinedImageSampler:
      return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  }
  return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
}

// Error checking utilities

constexpr auto IsSuccess(VkResult result) -> bool
{
  return result == VK_SUCCESS;
}

template<typename... AllowedWarnings>
constexpr auto IsSuccess(VkResult result,
                         std::span<const VkResult> allowed_results) -> bool
{
  if (result == VK_SUCCESS) {
    return true;
  }
  return std::ranges::any_of(
      allowed_results, [&](const auto res) -> auto { return res == result; });
}

constexpr auto Check(VkResult result) -> std::expected<void, VkResult>
{
  if (result != VK_SUCCESS) {
    return std::unexpected(result);
  }
  return {};
}

constexpr auto Check(VkResult result, std::span<const VkResult> allowed_results)
    -> std::expected<VkResult, VkResult>
{
  if (IsSuccess(result, allowed_results)) {
    return result;
  }
  return std::unexpected(result);
}

constexpr auto ToString(VkResult result) -> std::string_view
{
  switch (result) {
    case VK_SUCCESS:
      return "VK_SUCCESS";
    case VK_NOT_READY:
      return "VK_NOT_READY";
    case VK_TIMEOUT:
      return "VK_TIMEOUT";
    case VK_EVENT_SET:
      return "VK_EVENT_SET";
    case VK_EVENT_RESET:
      return "VK_EVENT_RESET";
    case VK_INCOMPLETE:
      return "VK_INCOMPLETE";
    case VK_ERROR_OUT_OF_HOST_MEMORY:
      return "VK_ERROR_OUT_OF_HOST_MEMORY";
    case VK_ERROR_OUT_OF_DEVICE_MEMORY:
      return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
    case VK_ERROR_INITIALIZATION_FAILED:
      return "VK_ERROR_INITIALIZATION_FAILED";
    case VK_ERROR_DEVICE_LOST:
      return "VK_ERROR_DEVICE_LOST";
    case VK_ERROR_MEMORY_MAP_FAILED:
      return "VK_ERROR_MEMORY_MAP_FAILED";
    case VK_ERROR_LAYER_NOT_PRESENT:
      return "VK_ERROR_LAYER_NOT_PRESENT";
    case VK_ERROR_EXTENSION_NOT_PRESENT:
      return "VK_ERROR_EXTENSION_NOT_PRESENT";
    case VK_ERROR_FEATURE_NOT_PRESENT:
      return "VK_ERROR_FEATURE_NOT_PRESENT";
    case VK_ERROR_INCOMPATIBLE_DRIVER:
      return "VK_ERROR_INCOMPATIBLE_DRIVER";
    case VK_ERROR_TOO_MANY_OBJECTS:
      return "VK_ERROR_TOO_MANY_OBJECTS";
    case VK_ERROR_FORMAT_NOT_SUPPORTED:
      return "VK_ERROR_FORMAT_NOT_SUPPORTED";
    case VK_ERROR_FRAGMENTED_POOL:
      return "VK_ERROR_FRAGMENTED_POOL";
    case VK_ERROR_UNKNOWN:
      return "VK_ERROR_UNKNOWN";
    case VK_ERROR_OUT_OF_POOL_MEMORY:
      return "VK_ERROR_OUT_OF_POOL_MEMORY";
    case VK_ERROR_INVALID_EXTERNAL_HANDLE:
      return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
    case VK_ERROR_FRAGMENTATION:
      return "VK_ERROR_FRAGMENTATION";
    case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS:
      return "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS";
    case VK_ERROR_SURFACE_LOST_KHR:
      return "VK_ERROR_SURFACE_LOST_KHR";
    case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
      return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
    case VK_SUBOPTIMAL_KHR:
      return "VK_SUBOPTIMAL_KHR";
    case VK_ERROR_OUT_OF_DATE_KHR:
      return "VK_ERROR_OUT_OF_DATE_KHR";
    case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
      return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
    case VK_ERROR_VALIDATION_FAILED_EXT:
      return "VK_ERROR_VALIDATION_FAILED_EXT";
    case VK_ERROR_INVALID_SHADER_NV:
      return "VK_ERROR_INVALID_SHADER_NV";
    case VK_PIPELINE_COMPILE_REQUIRED:
      return "VK_PIPELINE_COMPILE_REQUIRED";
    default:
      return "Unknown VkResult";
  }
}

}  // namespace VkUtils

#endif
