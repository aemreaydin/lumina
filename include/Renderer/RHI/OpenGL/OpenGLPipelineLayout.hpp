#ifndef RENDERER_RHI_OPENGL_OPENGLPIPELINELAYOUT_HPP
#define RENDERER_RHI_OPENGL_OPENGLPIPELINELAYOUT_HPP

#include <memory>
#include <vector>

#include "Renderer/RHI/RHIPipeline.hpp"

class OpenGLPipelineLayout : public RHIPipelineLayout
{
public:
  explicit OpenGLPipelineLayout(const PipelineLayoutDesc& desc);

  [[nodiscard]] auto GetSetLayouts() const
      -> const std::vector<std::shared_ptr<RHIDescriptorSetLayout>>&;

private:
  std::vector<std::shared_ptr<RHIDescriptorSetLayout>> m_SetLayouts;
};

#endif
