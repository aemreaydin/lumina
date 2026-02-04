#ifndef RENDERER_RHI_OPENGL_OPENGLDEVICE_HPP
#define RENDERER_RHI_OPENGL_OPENGLDEVICE_HPP

#include <memory>

#include <SDL3/SDL.h>

#include "Renderer/RHI/OpenGL/OpenGLCommandBuffer.hpp"
#include "Renderer/RHI/OpenGL/OpenGLSwapchain.hpp"
#include "Renderer/RHI/RHIDevice.hpp"
#include "Renderer/RHI/RenderPassInfo.hpp"

struct RendererConfig;

class OpenGLDevice final : public RHIDevice
{
public:
  OpenGLDevice() = default;
  OpenGLDevice(const OpenGLDevice&) = delete;
  OpenGLDevice(OpenGLDevice&&) = delete;
  auto operator=(const OpenGLDevice&) -> OpenGLDevice& = delete;
  auto operator=(OpenGLDevice&&) -> OpenGLDevice& = delete;
  ~OpenGLDevice() override = default;

  void Init(const RendererConfig& config, void* window) override;
  void CreateSwapchain(uint32_t width, uint32_t height) override;
  void Destroy() override;

  void BeginFrame() override;
  void EndFrame() override;
  void Present() override;
  void WaitIdle() override;

  [[nodiscard]] auto GetSwapchain() const -> RHISwapchain* override;
  [[nodiscard]] auto GetCurrentCommandBuffer() -> RHICommandBuffer* override;

  [[nodiscard]] auto CreateRenderTarget(const RenderTargetDesc& desc)
      -> std::unique_ptr<RHIRenderTarget> override;
  [[nodiscard]] auto CreateBuffer(const BufferDesc& desc)
      -> std::unique_ptr<RHIBuffer> override;
  [[nodiscard]] auto CreateTexture(const TextureDesc& desc)
      -> std::unique_ptr<RHITexture> override;
  [[nodiscard]] auto CreateSampler(const SamplerDesc& desc)
      -> std::unique_ptr<RHISampler> override;
  [[nodiscard]] auto CreateShaderModule(const ShaderModuleDesc& desc)
      -> std::unique_ptr<RHIShaderModule> override;
  [[nodiscard]] auto CreateGraphicsPipeline(const GraphicsPipelineDesc& desc)
      -> std::unique_ptr<RHIGraphicsPipeline> override;
  [[nodiscard]] auto CreateDescriptorSetLayout(
      const DescriptorSetLayoutDesc& desc)
      -> std::shared_ptr<RHIDescriptorSetLayout> override;
  [[nodiscard]] auto CreateDescriptorSet(
      const std::shared_ptr<RHIDescriptorSetLayout>& layout)
      -> std::unique_ptr<RHIDescriptorSet> override;
  [[nodiscard]] auto CreatePipelineLayout(
      const std::vector<std::shared_ptr<RHIDescriptorSetLayout>>& set_layouts)
      -> std::shared_ptr<RHIPipelineLayout> override;

private:
  std::unique_ptr<OpenGLSwapchain> m_Swapchain;
  std::unique_ptr<OpenGLCommandBuffer> m_CommandBuffer;
  SDL_Window* m_Window {nullptr};
  SDL_GLContext m_GLContext {nullptr};
  bool m_Initialized {false};
  bool m_DepthEnabled {false};
};

#endif
