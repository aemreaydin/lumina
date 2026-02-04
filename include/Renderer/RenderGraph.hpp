#ifndef RENDERER_RENDERGRAPH_HPP
#define RENDERER_RENDERGRAPH_HPP

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "Renderer/RHI/RHIRenderTarget.hpp"
#include "Renderer/RHI/RenderPassInfo.hpp"

class RHIDevice;
class RHICommandBuffer;

class RenderGraph
{
public:
  static constexpr const char* BACKBUFFER = "Backbuffer";

  struct ResourceDesc
  {
    std::string Name;
    uint32_t Width {0};
    uint32_t Height {0};
    TextureFormat ColorFormat {TextureFormat::RGBA8Srgb};
    TextureFormat DepthFormat {TextureFormat::Depth32F};
    bool HasDepth {true};
  };

  struct PassDesc
  {
    std::string Name;
    std::vector<std::string> Inputs;
    std::vector<std::string> Outputs;
    std::array<AttachmentInfo, kMaxColorAttachments> ColorAttachments {};
    uint32_t ColorAttachmentCount {1};
    bool UseDepth {true};
    DepthStencilInfo DepthStencil;
    std::function<void(RHICommandBuffer&)> Execute;
  };

  void AddResource(const ResourceDesc& desc);
  void AddPass(const PassDesc& desc);

  void Compile(RHIDevice& device);
  void Execute(RHICommandBuffer& cmd);

  void SetBackbufferSize(uint32_t width, uint32_t height);

  [[nodiscard]] auto IsCompiled() const -> bool;

  [[nodiscard]] auto GetTexture(const std::string& name) -> RHITexture*;
  [[nodiscard]] auto GetRenderTarget(const std::string& name)
      -> RHIRenderTarget*;

  void Resize(RHIDevice& device, uint32_t width, uint32_t height);
  void Clear();

private:
  struct Resource
  {
    ResourceDesc Desc;
    std::unique_ptr<RHIRenderTarget> Target;
  };

  struct Pass
  {
    PassDesc Desc;
    RenderPassInfo ResolvedInfo;
    DepthStencilInfo ResolvedDepthStencil;
  };

  std::unordered_map<std::string, Resource> m_Resources;
  std::vector<Pass> m_Passes;

  std::vector<size_t> m_ExecutionOrder;
  bool m_Compiled {false};
  uint32_t m_BackbufferWidth {0};
  uint32_t m_BackbufferHeight {0};

  void topologicalSort();
  void createResources(RHIDevice& device);
  void buildRenderPassInfos();
};

#endif
