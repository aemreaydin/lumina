#ifndef RENDERER_RHI_RHIDEVICE_HPP
#define RENDERER_RHI_RHIDEVICE_HPP

#include <cstdint>
#include <memory>

#include "Renderer/RHI/RHIVertexLayout.hpp"

struct RendererConfig;
class RHISwapchain;
class RHIBuffer;
class RHIShaderModule;
class RHIGraphicsPipeline;
class RHIDescriptorSetLayout;
class RHIDescriptorSet;
class RHIPipelineLayout;
struct BufferDesc;
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

  // Resource creation
  [[nodiscard]] virtual auto CreateBuffer(const BufferDesc& desc)
      -> std::unique_ptr<RHIBuffer> = 0;
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
  [[nodiscard]] virtual auto CreatePipelineLayout(const PipelineLayoutDesc& desc)
      -> std::shared_ptr<RHIPipelineLayout> = 0;

  // Drawing commands (recorded to current frame's command buffer)
  virtual void BindShaders(const RHIShaderModule* vertex_shader,
                           const RHIShaderModule* fragment_shader) = 0;
  virtual void BindVertexBuffer(const RHIBuffer& buffer, uint32_t binding) = 0;
  virtual void SetVertexInput(const VertexInputLayout& layout) = 0;
  virtual void SetPrimitiveTopology(PrimitiveTopology topology) = 0;
  virtual void BindDescriptorSet(uint32_t set_index,
                                 const RHIDescriptorSet& descriptor_set,
                                 const RHIPipelineLayout& layout) = 0;
  virtual void Draw(uint32_t vertex_count,
                    uint32_t instance_count,
                    uint32_t first_vertex,
                    uint32_t first_instance) = 0;

  RHIDevice(const RHIDevice&) = delete;
  RHIDevice(RHIDevice&&) = delete;
  auto operator=(const RHIDevice&) -> RHIDevice& = delete;
  auto operator=(RHIDevice&&) -> RHIDevice& = delete;
  virtual ~RHIDevice() = default;

protected:
  RHIDevice() = default;
};

#endif
