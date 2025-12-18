#ifndef RENDERER_RHI_RHIDESCRIPTORSET_HPP
#define RENDERER_RHI_RHIDESCRIPTORSET_HPP

#include <cstddef>
#include <cstdint>
#include <vector>

#include "Renderer/RHI/RHIShaderModule.hpp"

class RHIBuffer;
class RHITexture;
class RHISampler;

enum class DescriptorType : uint8_t
{
  UniformBuffer,
  Sampler,
  SampledImage,
  CombinedImageSampler,
  StorageBuffer
};

struct DescriptorBinding
{
  uint32_t Binding {0};
  DescriptorType Type {DescriptorType::UniformBuffer};
  ShaderStage Stages {ShaderStage::Vertex};
  uint32_t Count {1};
};

struct DescriptorSetLayoutDesc
{
  std::vector<DescriptorBinding> Bindings;
};

class RHIDescriptorSetLayout
{
public:
  RHIDescriptorSetLayout(const RHIDescriptorSetLayout&) = delete;
  RHIDescriptorSetLayout(RHIDescriptorSetLayout&&) = delete;
  auto operator=(const RHIDescriptorSetLayout&)
      -> RHIDescriptorSetLayout& = delete;
  auto operator=(RHIDescriptorSetLayout&&) -> RHIDescriptorSetLayout& = delete;
  virtual ~RHIDescriptorSetLayout() = default;

protected:
  RHIDescriptorSetLayout() = default;
};

class RHIDescriptorSet
{
public:
  RHIDescriptorSet(const RHIDescriptorSet&) = delete;
  RHIDescriptorSet(RHIDescriptorSet&&) = delete;
  auto operator=(const RHIDescriptorSet&) -> RHIDescriptorSet& = delete;
  auto operator=(RHIDescriptorSet&&) -> RHIDescriptorSet& = delete;
  virtual ~RHIDescriptorSet() = default;

  virtual void WriteBuffer(uint32_t binding,
                           RHIBuffer* buffer,
                           size_t offset,
                           size_t range) = 0;

  virtual void WriteCombinedImageSampler(uint32_t binding,
                                         RHITexture* texture,
                                         RHISampler* sampler) = 0;

protected:
  RHIDescriptorSet() = default;
};

#endif
