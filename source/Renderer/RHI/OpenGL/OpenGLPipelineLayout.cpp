#include "Renderer/RHI/OpenGL/OpenGLPipelineLayout.hpp"

#include "Core/Logger.hpp"

OpenGLPipelineLayout::OpenGLPipelineLayout(const PipelineLayoutDesc& desc)
    : m_SetLayouts(desc.SetLayouts)
{
  Logger::Trace("[OpenGL] Created pipeline layout with {} set layouts",
                m_SetLayouts.size());
}

auto OpenGLPipelineLayout::GetSetLayouts() const
    -> const std::vector<std::shared_ptr<RHIDescriptorSetLayout>>&
{
  return m_SetLayouts;
}
