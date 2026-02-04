#ifndef RENDERER_RHI_RHIRENDERTARGET_HPP
#define RENDERER_RHI_RHIRENDERTARGET_HPP

#include <cstdint>

#include "Renderer/RHI/RHITexture.hpp"

struct RenderTargetDesc
{
  uint32_t Width {0};
  uint32_t Height {0};
  TextureFormat ColorFormat {TextureFormat::RGBA8Srgb};
  TextureFormat DepthFormat {TextureFormat::Depth32F};
  bool HasDepth {true};
};

class RHIRenderTarget
{
public:
  RHIRenderTarget(const RHIRenderTarget&) = delete;
  RHIRenderTarget(RHIRenderTarget&&) = delete;
  auto operator=(const RHIRenderTarget&) -> RHIRenderTarget& = delete;
  auto operator=(RHIRenderTarget&&) -> RHIRenderTarget& = delete;
  virtual ~RHIRenderTarget() = default;

  [[nodiscard]] virtual auto GetWidth() const -> uint32_t = 0;
  [[nodiscard]] virtual auto GetHeight() const -> uint32_t = 0;
  [[nodiscard]] virtual auto GetColorTexture() -> RHITexture* = 0;
  [[nodiscard]] virtual auto GetDepthTexture() -> RHITexture* = 0;

protected:
  RHIRenderTarget() = default;
};

#endif
