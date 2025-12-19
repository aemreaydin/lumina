#include <stdexcept>

#include "Renderer/RHI/Vulkan/VulkanDescriptorSet.hpp"

#include "Core/Logger.hpp"
#include "Renderer/RHI/Vulkan/VulkanBuffer.hpp"
#include "Renderer/RHI/Vulkan/VulkanDevice.hpp"
#include "Renderer/RHI/Vulkan/VulkanSampler.hpp"
#include "Renderer/RHI/Vulkan/VulkanTexture.hpp"
#include "Renderer/RHI/Vulkan/VulkanUtils.hpp"

VulkanDescriptorSetLayout::VulkanDescriptorSetLayout(
    const VulkanDevice& device, const DescriptorSetLayoutDesc& desc)
    : m_Device(device)
    , m_Bindings(desc.Bindings)
{
  std::vector<VkDescriptorSetLayoutBinding> vk_bindings;
  vk_bindings.reserve(desc.Bindings.size());

  for (const auto& binding : desc.Bindings) {
    VkDescriptorSetLayoutBinding vk_binding {};
    vk_binding.binding = binding.Binding;
    vk_binding.descriptorType = VkUtils::ToVkDescriptorType(binding.Type);
    vk_binding.descriptorCount = binding.Count;
    vk_binding.stageFlags = VkUtils::ToVkShaderStageFlags(binding.Stages);
    vk_binding.pImmutableSamplers = nullptr;
    vk_bindings.push_back(vk_binding);
  }

  VkDescriptorSetLayoutCreateInfo layout_info {};
  layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layout_info.bindingCount = static_cast<uint32_t>(vk_bindings.size());
  layout_info.pBindings = vk_bindings.data();

  if (auto result = VkUtils::Check(vkCreateDescriptorSetLayout(
          m_Device.GetVkDevice(), &layout_info, nullptr, &m_Layout));
      !result)
  {
    throw std::runtime_error("Failed to create Vulkan descriptor set layout");
  }

  Logger::Trace("[Vulkan] Created descriptor set layout with {} bindings",
                desc.Bindings.size());
}

VulkanDescriptorSetLayout::~VulkanDescriptorSetLayout()
{
  if (m_Layout != VK_NULL_HANDLE) {
    vkDestroyDescriptorSetLayout(m_Device.GetVkDevice(), m_Layout, nullptr);
    Logger::Trace("[Vulkan] Destroyed descriptor set layout");
  }
}

VulkanDescriptorSet::VulkanDescriptorSet(
    const VulkanDevice& device,
    VkDescriptorPool pool,
    const std::shared_ptr<RHIDescriptorSetLayout>& layout)
    : m_Device(device)
    , m_Pool(pool)
    , m_Layout(layout)
{
  const auto* vk_layout =
      dynamic_cast<const VulkanDescriptorSetLayout*>(layout.get());
  if (vk_layout == nullptr) {
    throw std::runtime_error("Invalid descriptor set layout for Vulkan");
  }

  VkDescriptorSetLayout vk_set_layout = vk_layout->GetVkDescriptorSetLayout();

  VkDescriptorSetAllocateInfo alloc_info {};
  alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  alloc_info.descriptorPool = m_Pool;
  alloc_info.descriptorSetCount = 1;
  alloc_info.pSetLayouts = &vk_set_layout;

  if (auto result = VkUtils::Check(vkAllocateDescriptorSets(
          m_Device.GetVkDevice(), &alloc_info, &m_DescriptorSet));
      !result)
  {
    throw std::runtime_error("Failed to allocate Vulkan descriptor set");
  }

  Logger::Trace("[Vulkan] Allocated descriptor set");
}

VulkanDescriptorSet::~VulkanDescriptorSet()
{
  // Descriptor sets are freed when the pool is reset/destroyed
  // We don't free individual sets since we don't use FREE_DESCRIPTOR_SET_BIT
  Logger::Trace("[Vulkan] Descriptor set will be freed with pool");
}

void VulkanDescriptorSet::WriteBuffer(uint32_t binding,
                                      RHIBuffer* buffer,
                                      size_t offset,
                                      size_t range)
{
  const auto* vk_buffer = dynamic_cast<const VulkanBuffer*>(buffer);
  if (vk_buffer == nullptr) {
    Logger::Error("[Vulkan] WriteBuffer: Invalid buffer");
    return;
  }

  VkDescriptorBufferInfo buffer_info {};
  buffer_info.buffer = vk_buffer->GetVkBuffer();
  buffer_info.offset = offset;
  buffer_info.range = range == 0 ? vk_buffer->GetSize() : range;

  VkWriteDescriptorSet write {};
  write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write.dstSet = m_DescriptorSet;
  write.dstBinding = binding;
  write.dstArrayElement = 0;
  write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  write.descriptorCount = 1;
  write.pBufferInfo = &buffer_info;

  vkUpdateDescriptorSets(m_Device.GetVkDevice(), 1, &write, 0, nullptr);

  Logger::Trace("[Vulkan] Descriptor set: wrote buffer to binding {}", binding);
}

void VulkanDescriptorSet::WriteCombinedImageSampler(uint32_t binding,
                                                    RHITexture* texture,
                                                    RHISampler* sampler)
{
  const auto* vk_texture = dynamic_cast<const VulkanTexture*>(texture);
  const auto* vk_sampler = dynamic_cast<const VulkanSampler*>(sampler);

  if (vk_texture == nullptr || vk_sampler == nullptr) {
    Logger::Error(
        "[Vulkan] WriteCombinedImageSampler: Invalid texture or sampler");
    return;
  }

  VkDescriptorImageInfo image_info {};
  image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  image_info.imageView = vk_texture->GetVkImageView();
  image_info.sampler = vk_sampler->GetVkSampler();

  VkWriteDescriptorSet write {};
  write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write.dstSet = m_DescriptorSet;
  write.dstBinding = binding;
  write.dstArrayElement = 0;
  write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  write.descriptorCount = 1;
  write.pImageInfo = &image_info;

  vkUpdateDescriptorSets(m_Device.GetVkDevice(), 1, &write, 0, nullptr);

  Logger::Trace("[Vulkan] Descriptor set: wrote texture/sampler to binding {}",
                binding);
}
