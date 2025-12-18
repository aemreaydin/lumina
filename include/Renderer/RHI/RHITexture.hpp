#ifndef RENDERER_RHI_RHITEXTURE_HPP
#define RENDERER_RHI_RHITEXTURE_HPP

#include <cstddef>
#include <cstdint>

enum class TextureFormat : uint8_t
{
  R8Unorm,
  RG8Unorm,
  RGB8Unorm,
  RGB8Srgb,
  RGBA8Unorm,
  RGBA8Srgb,
  BGRA8Unorm,
  RGBA16F,
  RGBA32F,
  Depth24Stencil8,
  Depth32F
};

enum class TextureUsage : uint8_t
{
  Sampled = 1 << 0,
  Storage = 1 << 1,
  TransferDst = 1 << 2,
  TransferSrc = 1 << 3,
  ColorAttachment = 1 << 4,
  DepthStencilAttachment = 1 << 5
};

constexpr auto operator|(TextureUsage lhs, TextureUsage rhs) -> TextureUsage
{
  return static_cast<TextureUsage>(static_cast<uint8_t>(lhs)
                                   | static_cast<uint8_t>(rhs));
}

constexpr auto operator&(TextureUsage lhs, TextureUsage rhs) -> TextureUsage
{
  return static_cast<TextureUsage>(static_cast<uint8_t>(lhs)
                                   & static_cast<uint8_t>(rhs));
}

struct TextureDesc
{
  uint32_t Width {0};
  uint32_t Height {0};
  TextureFormat Format {TextureFormat::RGBA8Unorm};
  TextureUsage Usage {TextureUsage::Sampled};
  uint32_t MipLevels {1};
};

class RHITexture
{
public:
  RHITexture(const RHITexture&) = delete;
  RHITexture(RHITexture&&) = delete;
  auto operator=(const RHITexture&) -> RHITexture& = delete;
  auto operator=(RHITexture&&) -> RHITexture& = delete;
  virtual ~RHITexture() = default;

  [[nodiscard]] virtual auto GetWidth() const -> uint32_t = 0;
  [[nodiscard]] virtual auto GetHeight() const -> uint32_t = 0;
  [[nodiscard]] virtual auto GetFormat() const -> TextureFormat = 0;

  virtual void Upload(const void* data, size_t size) = 0;

protected:
  RHITexture() = default;
};

#endif
