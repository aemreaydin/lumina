#ifndef RENDERER_RHI_RHISHADERMODULE_HPP
#define RENDERER_RHI_RHISHADERMODULE_HPP

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

class RHIDescriptorSetLayout;

enum class ShaderStage : uint8_t
{
  Vertex,
  Fragment,
  Compute
};

constexpr auto ToString(ShaderStage stage) -> const char*
{
  switch (stage) {
    case ShaderStage::Vertex:
      return "vertex";
    case ShaderStage::Fragment:
      return "fragment";
    case ShaderStage::Compute:
      return "compute";
  }
  return "unknown";
}

struct ShaderModuleDesc
{
  ShaderStage Stage {ShaderStage::Vertex};
  std::vector<uint32_t> SPIRVCode;
  std::string EntryPoint {"main"};
  std::vector<std::shared_ptr<RHIDescriptorSetLayout>> SetLayouts;
};

class RHIShaderModule
{
public:
  RHIShaderModule(const RHIShaderModule&) = delete;
  RHIShaderModule(RHIShaderModule&&) = delete;
  auto operator=(const RHIShaderModule&) -> RHIShaderModule& = delete;
  auto operator=(RHIShaderModule&&) -> RHIShaderModule& = delete;
  virtual ~RHIShaderModule() = default;

  [[nodiscard]] virtual auto GetStage() const -> ShaderStage = 0;

protected:
  RHIShaderModule() = default;
};

#endif
