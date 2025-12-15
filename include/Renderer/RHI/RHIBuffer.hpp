#ifndef RENDERER_RHI_RHIBUFFER_HPP
#define RENDERER_RHI_RHIBUFFER_HPP

#include <cstddef>
#include <cstdint>

enum class BufferUsage : uint8_t
{
  Vertex = 1 << 0,
  Index = 1 << 1,
  Uniform = 1 << 2,
  TransferSrc = 1 << 3,
  TransferDst = 1 << 4
};

constexpr auto operator|(BufferUsage lhs, BufferUsage rhs) -> BufferUsage
{
  return static_cast<BufferUsage>(static_cast<uint32_t>(lhs)
                                  | static_cast<uint32_t>(rhs));
}

constexpr auto operator&(BufferUsage lhs, BufferUsage rhs) -> BufferUsage
{
  return static_cast<BufferUsage>(static_cast<uint32_t>(lhs)
                                  & static_cast<uint32_t>(rhs));
}

struct BufferDesc
{
  size_t Size {0};
  BufferUsage Usage {BufferUsage::Vertex};
  bool CPUVisible {true};
};

class RHIBuffer
{
public:
  RHIBuffer(const RHIBuffer&) = delete;
  RHIBuffer(RHIBuffer&&) = delete;
  auto operator=(const RHIBuffer&) -> RHIBuffer& = delete;
  auto operator=(RHIBuffer&&) -> RHIBuffer& = delete;
  virtual ~RHIBuffer() = default;

  [[nodiscard]] virtual auto Map() -> void* = 0;

  virtual void Unmap() = 0;

  virtual void Upload(const void* data, size_t size, size_t offset) = 0;

  [[nodiscard]] virtual auto GetSize() const -> size_t = 0;

protected:
  RHIBuffer() = default;
};

#endif
