#include <format>
#include <stdexcept>

#include "Renderer/RHI/Vulkan/VulkanSampler.hpp"

#include "Core/Logger.hpp"
#include "Renderer/RHI/Vulkan/VulkanDevice.hpp"
#include "Renderer/RHI/Vulkan/VulkanUtils.hpp"

static auto ToVkFilter(Filter filter) -> VkFilter
{
  switch (filter) {
    case Filter::Nearest:
      return VK_FILTER_NEAREST;
    case Filter::Linear:
      return VK_FILTER_LINEAR;
  }
  return VK_FILTER_LINEAR;
}

static auto ToVkMipmapMode(Filter filter) -> VkSamplerMipmapMode
{
  switch (filter) {
    case Filter::Nearest:
      return VK_SAMPLER_MIPMAP_MODE_NEAREST;
    case Filter::Linear:
      return VK_SAMPLER_MIPMAP_MODE_LINEAR;
  }
  return VK_SAMPLER_MIPMAP_MODE_LINEAR;
}

static auto ToVkAddressMode(SamplerAddressMode mode) -> VkSamplerAddressMode
{
  switch (mode) {
    case SamplerAddressMode::Repeat:
      return VK_SAMPLER_ADDRESS_MODE_REPEAT;
    case SamplerAddressMode::MirroredRepeat:
      return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    case SamplerAddressMode::ClampToEdge:
      return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    case SamplerAddressMode::ClampToBorder:
      return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
  }
  return VK_SAMPLER_ADDRESS_MODE_REPEAT;
}

VulkanSampler::VulkanSampler(const VulkanDevice& device,
                             const SamplerDesc& desc)
    : m_Device(device)
{
  VkSamplerCreateInfo sampler_info = {};
  sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  sampler_info.magFilter = ToVkFilter(desc.MagFilter);
  sampler_info.minFilter = ToVkFilter(desc.MinFilter);
  sampler_info.mipmapMode = ToVkMipmapMode(desc.MipFilter);
  sampler_info.addressModeU = ToVkAddressMode(desc.AddressModeU);
  sampler_info.addressModeV = ToVkAddressMode(desc.AddressModeV);
  sampler_info.addressModeW =
      ToVkAddressMode(desc.AddressModeU);  // Use U for W
  sampler_info.mipLodBias = 0.0F;
  sampler_info.anisotropyEnable = desc.EnableAnisotropy ? VK_TRUE : VK_FALSE;
  sampler_info.maxAnisotropy = desc.MaxAnisotropy;
  sampler_info.compareEnable = VK_FALSE;
  sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
  sampler_info.minLod = 0.0F;
  sampler_info.maxLod = VK_LOD_CLAMP_NONE;
  sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  sampler_info.unnormalizedCoordinates = VK_FALSE;

  if (auto result = VkUtils::Check(vkCreateSampler(
          m_Device.GetVkDevice(), &sampler_info, nullptr, &m_Sampler));
      !result)
  {
    throw std::runtime_error(std::format("Failed to create Vulkan sampler: {}",
                                         VkUtils::ToString(result.error())));
  }

  Logger::Trace("[Vulkan] Created sampler");
}

VulkanSampler::~VulkanSampler()
{
  if (m_Sampler != VK_NULL_HANDLE) {
    vkDestroySampler(m_Device.GetVkDevice(), m_Sampler, nullptr);
  }
  Logger::Trace("[Vulkan] Destroyed sampler");
}
