#ifndef RENDERER_RHI_RHISHADERMODULE_HPP
#define RENDERER_RHI_RHISHADERMODULE_HPP

#include <cstdint>
#include <memory>
#include <ranges>
#include <string>
#include <vector>

class RHIDescriptorSetLayout;

#ifdef _WIN32
enum class ShaderStage : uint8_t
#elifdef __linux__
enum class [[clang::flag_enum]] ShaderStage : uint8_t
#endif
{
  Vertex = 1 << 0,
  Fragment = 1 << 1,
  Compute = 1 << 2,
};

constexpr auto operator|(ShaderStage lhs, ShaderStage rhs) -> ShaderStage
{
  return static_cast<ShaderStage>(static_cast<uint8_t>(lhs)
                                  | static_cast<uint8_t>(rhs));
}

constexpr auto operator&(ShaderStage lhs, ShaderStage rhs) -> ShaderStage
{
  return static_cast<ShaderStage>(static_cast<uint8_t>(lhs)
                                  & static_cast<uint8_t>(rhs));
}

constexpr auto ToString(ShaderStage stage) -> std::string
{
  std::vector<std::string> res_vec;
  if ((stage & ShaderStage::Vertex) == ShaderStage::Vertex) {
    res_vec.emplace_back("vertex");
  }
  if ((stage & ShaderStage::Fragment) == ShaderStage::Fragment) {
    res_vec.emplace_back("vertex");
  }
  if ((stage & ShaderStage::Compute) == ShaderStage::Compute) {
    res_vec.emplace_back("vertex");
  }
  if (res_vec.empty()) {
    return "unknown";
  }
  return std::ranges::views::join_with(res_vec, ',')
      | std::ranges::to<std::string>();
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
