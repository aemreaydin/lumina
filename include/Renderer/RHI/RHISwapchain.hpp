#ifndef RENDERER_RHI_RHISWAPCHAIN_HPP
#define RENDERER_RHI_RHISWAPCHAIN_HPP

#include <cstdint>

class RHISwapchain
{
public:
  RHISwapchain(const RHISwapchain&) = delete;
  RHISwapchain(RHISwapchain&&) = delete;
  auto operator=(const RHISwapchain&) -> RHISwapchain& = delete;
  auto operator=(RHISwapchain&&) -> RHISwapchain& = delete;
  virtual ~RHISwapchain() = default;

  [[nodiscard]] virtual auto AcquireNextImage() -> uint32_t = 0;
  virtual void Present(uint32_t image_index) = 0;
  virtual void Resize(uint32_t width, uint32_t height) = 0;

  [[nodiscard]] virtual auto GetWidth() const -> uint32_t { return m_Width; }

  [[nodiscard]] virtual auto GetHeight() const -> uint32_t { return m_Height; }

protected:
  RHISwapchain() = default;
  uint32_t m_Width {0};
  uint32_t m_Height {0};
};

#endif
