#ifndef RENDERER_RHI_RENDERPASSINFO_HPP
#define RENDERER_RHI_RENDERPASSINFO_HPP

#include <cstdint>

class RHIRenderTarget;

enum class LoadOp : uint8_t
{
  Load,  // Preserve existing contents
  Clear,  // Clear to specified color
  DontCare  // Contents undefined
};

enum class StoreOp : uint8_t
{
  Store,  // Store results to memory
  DontCare  // Don't need to store
};

struct ClearColorValue
{
  float R {0.0F};
  float G {0.0F};
  float B {0.0F};
  float A {1.0F};
};

struct ClearDepthStencilValue
{
  float Depth {1.0F};
  uint32_t Stencil {0};
};

struct AttachmentInfo
{
  LoadOp ColorLoadOp {LoadOp::Clear};
  StoreOp ColorStoreOp {StoreOp::Store};
  ClearColorValue ClearColor;
};

struct DepthStencilInfo
{
  LoadOp DepthLoadOp {LoadOp::Clear};
  StoreOp DepthStoreOp {StoreOp::Store};
  LoadOp StencilLoadOp {LoadOp::DontCare};
  StoreOp StencilStoreOp {StoreOp::DontCare};
  ClearDepthStencilValue ClearDepthStencil;
};

struct RenderPassInfo
{
  RHIRenderTarget* RenderTarget {nullptr};  // nullptr = swapchain
  AttachmentInfo ColorAttachment;
  DepthStencilInfo* DepthStencilAttachment {nullptr};  // nullptr = no depth
  uint32_t Width {0};
  uint32_t Height {0};
};

#endif
