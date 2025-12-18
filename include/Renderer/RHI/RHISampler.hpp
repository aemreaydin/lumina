#ifndef RENDERER_RHI_RHISAMPLER_HPP
#define RENDERER_RHI_RHISAMPLER_HPP

#include <cstdint>

enum class Filter : uint8_t
{
  Nearest,
  Linear
};

enum class SamplerAddressMode : uint8_t
{
  Repeat,
  MirroredRepeat,
  ClampToEdge,
  ClampToBorder
};

struct SamplerDesc
{
  Filter MinFilter {Filter::Linear};
  Filter MagFilter {Filter::Linear};
  Filter MipFilter {Filter::Linear};
  SamplerAddressMode AddressModeU {SamplerAddressMode::Repeat};
  SamplerAddressMode AddressModeV {SamplerAddressMode::Repeat};
  float MaxAnisotropy {1.0F};
  bool EnableAnisotropy {false};
};

class RHISampler
{
public:
  RHISampler(const RHISampler&) = delete;
  RHISampler(RHISampler&&) = delete;
  auto operator=(const RHISampler&) -> RHISampler& = delete;
  auto operator=(RHISampler&&) -> RHISampler& = delete;
  virtual ~RHISampler() = default;

protected:
  RHISampler() = default;
};

#endif
