#ifndef RENDERER_RHI_RHIPIPELINE_HPP
#define RENDERER_RHI_RHIPIPELINE_HPP

#include <memory>
#include <vector>

#include "Renderer/RHI/RHIShaderModule.hpp"
#include "Renderer/RHI/RHIVertexLayout.hpp"

class RHIDescriptorSetLayout;

struct PipelineLayoutDesc
{
  std::vector<std::shared_ptr<RHIDescriptorSetLayout>> SetLayouts;
  std::vector<PushConstant> PushConstants;
};

class RHIPipelineLayout
{
public:
  RHIPipelineLayout(const RHIPipelineLayout&) = delete;
  RHIPipelineLayout(RHIPipelineLayout&&) = delete;
  auto operator=(const RHIPipelineLayout&) -> RHIPipelineLayout& = delete;
  auto operator=(RHIPipelineLayout&&) -> RHIPipelineLayout& = delete;
  virtual ~RHIPipelineLayout() = default;

protected:
  RHIPipelineLayout() = default;
};

struct GraphicsPipelineDesc
{
  RHIShaderModule* VertexShader {nullptr};
  RHIShaderModule* FragmentShader {nullptr};
  VertexInputLayout VertexLayout;
  PrimitiveTopology Topology {PrimitiveTopology::TriangleList};
  std::shared_ptr<RHIPipelineLayout> Layout;
  uint32_t Width {0};
  uint32_t Height {0};
};

class RHIGraphicsPipeline
{
public:
  RHIGraphicsPipeline(const RHIGraphicsPipeline&) = delete;
  RHIGraphicsPipeline(RHIGraphicsPipeline&&) = delete;
  auto operator=(const RHIGraphicsPipeline&) -> RHIGraphicsPipeline& = delete;
  auto operator=(RHIGraphicsPipeline&&) -> RHIGraphicsPipeline& = delete;
  virtual ~RHIGraphicsPipeline() = default;

protected:
  RHIGraphicsPipeline() = default;
};

#endif
