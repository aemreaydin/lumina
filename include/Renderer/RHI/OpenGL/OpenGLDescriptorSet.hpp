#ifndef RENDERER_RHI_OPENGL_OPENGLDESCRIPTORSET_HPP
#define RENDERER_RHI_OPENGL_OPENGLDESCRIPTORSET_HPP

#include <cstdint>
#include <memory>
#include <span>
#include <unordered_map>

#include "Renderer/RHI/RHIDescriptorSet.hpp"

class OpenGLBuffer;
class OpenGLTexture;
class OpenGLSampler;

class OpenGLDescriptorSetLayout : public RHIDescriptorSetLayout
{
public:
  explicit OpenGLDescriptorSetLayout(const DescriptorSetLayoutDesc& desc);

  [[nodiscard]] auto GetBindings() const
      -> const std::vector<DescriptorBinding>&;

private:
  std::vector<DescriptorBinding> m_Bindings;
};

struct OpenGLBufferBinding
{
  const OpenGLBuffer* Buffer {nullptr};
  size_t Offset {0};
  size_t Range {0};
};

struct OpenGLTextureBinding
{
  const OpenGLTexture* Texture {nullptr};
  const OpenGLSampler* Sampler {nullptr};
};

class OpenGLDescriptorSet : public RHIDescriptorSet
{
public:
  explicit OpenGLDescriptorSet(
      const std::shared_ptr<RHIDescriptorSetLayout>& layout);

  void WriteBuffer(uint32_t binding,
                   RHIBuffer* buffer,
                   size_t offset,
                   size_t range) override;

  void WriteCombinedImageSampler(uint32_t binding,
                                 RHITexture* texture,
                                 RHISampler* sampler) override;

  void Bind() const;
  void Bind(std::span<const uint32_t> dynamic_offsets) const;

private:
  std::shared_ptr<RHIDescriptorSetLayout> m_Layout;
  std::unordered_map<uint32_t, OpenGLBufferBinding> m_BufferBindings;
  std::unordered_map<uint32_t, OpenGLTextureBinding> m_TextureBindings;
};

#endif
