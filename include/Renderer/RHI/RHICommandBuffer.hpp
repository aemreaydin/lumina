#ifndef RENDERER_RHI_RHICOMMANDBUFFER_HPP
#define RENDERER_RHI_RHICOMMANDBUFFER_HPP

#include <cstdint>
#include <span>

#include "Renderer/RHI/RHIPipeline.hpp"
#include "Renderer/RHI/RHIShaderModule.hpp"
#include "Renderer/RHI/RHIVertexLayout.hpp"

class RHIBuffer;
class RHIDescriptorSet;
struct RenderPassInfo;

class RHICommandBuffer
{
public:
  RHICommandBuffer() = default;
  RHICommandBuffer(const RHICommandBuffer&) = delete;
  RHICommandBuffer(RHICommandBuffer&&) = delete;
  auto operator=(const RHICommandBuffer&) -> RHICommandBuffer& = delete;
  auto operator=(RHICommandBuffer&&) -> RHICommandBuffer& = delete;
  virtual ~RHICommandBuffer() = default;

  // Drawing commands
  virtual void BindShaders(const RHIShaderModule* vertex_shader,
                           const RHIShaderModule* fragment_shader) = 0;
  virtual void BindVertexBuffer(const RHIBuffer& buffer, uint32_t binding) = 0;
  virtual void BindIndexBuffer(const RHIBuffer& buffer) = 0;
  virtual void SetVertexInput(const VertexInputLayout& layout) = 0;
  virtual void SetPrimitiveTopology(PrimitiveTopology topology) = 0;
  virtual void SetPolygonMode(PolygonMode mode) = 0;
  virtual void BindDescriptorSet(
      uint32_t set_index,
      const RHIDescriptorSet& descriptor_set,
      const RHIPipelineLayout& layout,
      std::span<const uint32_t> dynamic_offsets = {}) = 0;
  virtual void Draw(uint32_t vertex_count,
                    uint32_t instance_count,
                    uint32_t first_vertex,
                    uint32_t first_instance) = 0;
  virtual void DrawIndexed(uint32_t index_count,
                           uint32_t instance_count,
                           uint32_t first_index,
                           int32_t vertex_offset,
                           uint32_t first_instance) = 0;
};

#endif
