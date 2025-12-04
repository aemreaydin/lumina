#ifndef RENDERER_RHI_RHIDEVICE_HPP
#define RENDERER_RHI_RHIDEVICE_HPP

#include <cstdint>
#include <memory>

struct RendererConfig;
class RHISwapchain;

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

  [[nodiscard]] virtual auto GetSwapchain() -> RHISwapchain* = 0;

  RHIDevice(const RHIDevice&) = delete;
  RHIDevice(RHIDevice&&) = delete;
  auto operator=(const RHIDevice&) -> RHIDevice& = delete;
  auto operator=(RHIDevice&&) -> RHIDevice& = delete;
  virtual ~RHIDevice() = default;

protected:
  RHIDevice() = default;
};

#endif
