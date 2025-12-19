#ifndef RENDERER_RHI_RHIDEVICE_HPP
#define RENDERER_RHI_RHIDEVICE_HPP

#include <cstdint>
#include <memory>

struct RendererConfig;
class RHISwapchain;
class RHIBuffer;
class RHITexture;
class RHISampler;
class RHIShaderModule;
class RHIGraphicsPipeline;
class RHIDescriptorSetLayout;
class RHIDescriptorSet;
class RHIPipelineLayout;
class RHICommandBuffer;
struct BufferDesc;
struct TextureDesc;
struct SamplerDesc;
struct ShaderModuleDesc;
struct GraphicsPipelineDesc;
struct DescriptorSetLayoutDesc;
struct PipelineLayoutDesc;

class RHIDevice
{
public:
  static auto Create(const RendererConfig& config)
      -> std::unique_ptr<RHIDevice>;

  virtual void Init(const RendererConfig& config, void* window) = 0;
  virtual void CreateSwapchain(uint32_t width, uint32_t height) = 0;
  virtual void Destroy() = 0;

  virtual void BeginFrame() = 0;
  virtual void EndFrame() = 0;
  virtual void Present() = 0;
  virtual void WaitIdle() = 0;

  [[nodiscard]] virtual auto GetSwapchain() -> RHISwapchain* = 0;
  [[nodiscard]] virtual auto GetCurrentCommandBuffer() -> RHICommandBuffer* = 0;

  // Resource creation
  [[nodiscard]] virtual auto CreateBuffer(const BufferDesc& desc)
      -> std::unique_ptr<RHIBuffer> = 0;
  [[nodiscard]] virtual auto CreateTexture(const TextureDesc& desc)
      -> std::unique_ptr<RHITexture> = 0;
  [[nodiscard]] virtual auto CreateSampler(const SamplerDesc& desc)
      -> std::unique_ptr<RHISampler> = 0;
  [[nodiscard]] virtual auto CreateShaderModule(const ShaderModuleDesc& desc)
      -> std::unique_ptr<RHIShaderModule> = 0;
  [[nodiscard]] virtual auto CreateGraphicsPipeline(
      const GraphicsPipelineDesc& desc)
      -> std::unique_ptr<RHIGraphicsPipeline> = 0;

  // Descriptor and pipeline layout creation
  [[nodiscard]] virtual auto CreateDescriptorSetLayout(
      const DescriptorSetLayoutDesc& desc)
      -> std::shared_ptr<RHIDescriptorSetLayout> = 0;
  [[nodiscard]] virtual auto CreateDescriptorSet(
      const std::shared_ptr<RHIDescriptorSetLayout>& layout)
      -> std::unique_ptr<RHIDescriptorSet> = 0;
  [[nodiscard]] virtual auto CreatePipelineLayout(
      const PipelineLayoutDesc& desc) -> std::shared_ptr<RHIPipelineLayout> = 0;

  RHIDevice(const RHIDevice&) = delete;
  RHIDevice(RHIDevice&&) = delete;
  auto operator=(const RHIDevice&) -> RHIDevice& = delete;
  auto operator=(RHIDevice&&) -> RHIDevice& = delete;
  virtual ~RHIDevice() = default;

protected:
  RHIDevice() = default;
};

#endif
