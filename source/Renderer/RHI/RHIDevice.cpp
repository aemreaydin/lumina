#include <stdexcept>

#include "Renderer/RHI/RHIDevice.hpp"

#include "Renderer/RHI/OpenGL/OpenGLDevice.hpp"
#include "Renderer/RHI/Vulkan/VulkanDevice.hpp"
#include "Renderer/RendererConfig.hpp"

auto RHIDevice::Create(const RendererConfig& config)
    -> std::unique_ptr<RHIDevice>
{
  switch (config.API) {
    case RenderAPI::OpenGL:
      return std::make_unique<OpenGLDevice>();

    case RenderAPI::Vulkan:
      return std::make_unique<VulkanDevice>();

    default:
      throw std::runtime_error("Unsupported render API");
  }
}
