#include "Renderer/RHI/OpenGL/OpenGLPipelineLayout.hpp"

#include "Core/Logger.hpp"

OpenGLPipelineLayout::OpenGLPipelineLayout(
    const std::vector<std::shared_ptr<RHIDescriptorSetLayout>>& set_layouts)
    : m_SetLayouts(set_layouts)
{
  Logger::Trace("[OpenGL] Created pipeline layout with {} set layouts",
                m_SetLayouts.size());
}

auto OpenGLPipelineLayout::GetSetLayouts() const
    -> const std::vector<std::shared_ptr<RHIDescriptorSetLayout>>&
{
  return m_SetLayouts;
}
