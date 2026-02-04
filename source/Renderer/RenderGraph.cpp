#include "Renderer/RenderGraph.hpp"

#include <algorithm>
#include <format>
#include <stdexcept>

#include "Core/Logger.hpp"
#include "Renderer/RHI/RHICommandBuffer.hpp"
#include "Renderer/RHI/RHIDevice.hpp"
#include "Renderer/RHI/RHISwapchain.hpp"

void RenderGraph::AddResource(const ResourceDesc& desc)
{
  m_Resources[desc.Name] = Resource {.Desc = desc, .Target = nullptr};
  m_Compiled = false;
}

void RenderGraph::AddPass(const PassDesc& desc)
{
  m_Passes.push_back(Pass {.Desc = desc, .ResolvedInfo = {}, .ResolvedDepthStencil = {}});
  m_Compiled = false;
}

void RenderGraph::Compile(RHIDevice& device)
{
  topologicalSort();
  createResources(device);
  buildRenderPassInfos();
  m_Compiled = true;
  Logger::Info("[RenderGraph] Compiled {} passes, {} resources",
               m_Passes.size(),
               m_Resources.size());
}

void RenderGraph::Execute(RHICommandBuffer& cmd)
{
  for (size_t idx : m_ExecutionOrder) {
    auto& pass = m_Passes[idx];

    // Resolve backbuffer dimensions each frame
    if (pass.ResolvedInfo.RenderTarget == nullptr) {
      pass.ResolvedInfo.Width = m_BackbufferWidth;
      pass.ResolvedInfo.Height = m_BackbufferHeight;
    }

    cmd.BeginRenderPass(pass.ResolvedInfo);
    pass.Desc.Execute(cmd);
    cmd.EndRenderPass();
  }
}

void RenderGraph::SetBackbufferSize(uint32_t width, uint32_t height)
{
  m_BackbufferWidth = width;
  m_BackbufferHeight = height;
}

auto RenderGraph::IsCompiled() const -> bool
{
  return m_Compiled;
}

auto RenderGraph::GetTexture(const std::string& name) -> RHITexture*
{
  auto it = m_Resources.find(name);
  if (it == m_Resources.end() || it->second.Target == nullptr) {
    return nullptr;
  }
  return it->second.Target->GetColorTexture();
}

auto RenderGraph::GetRenderTarget(const std::string& name) -> RHIRenderTarget*
{
  auto it = m_Resources.find(name);
  if (it == m_Resources.end()) {
    return nullptr;
  }
  return it->second.Target.get();
}

void RenderGraph::Resize(RHIDevice& device, uint32_t width, uint32_t height)
{
  for (auto& [name, resource] : m_Resources) {
    if (name == BACKBUFFER) {
      continue;
    }
    resource.Desc.Width = width;
    resource.Desc.Height = height;
    resource.Target.reset();
  }
  createResources(device);
  buildRenderPassInfos();
}

void RenderGraph::Clear()
{
  m_Resources.clear();
  m_Passes.clear();
  m_ExecutionOrder.clear();
  m_Compiled = false;
}

void RenderGraph::topologicalSort()
{
  const size_t pass_count = m_Passes.size();

  std::unordered_map<std::string, size_t> writer;
  for (size_t i = 0; i < pass_count; ++i) {
    for (const auto& output : m_Passes[i].Desc.Outputs) {
      writer[output] = i;
    }
  }

  std::vector<std::vector<size_t>> adj(pass_count);
  std::vector<size_t> in_degree(pass_count, 0);

  for (size_t i = 0; i < pass_count; ++i) {
    for (const auto& input : m_Passes[i].Desc.Inputs) {
      auto it = writer.find(input);
      if (it != writer.end()) {
        adj[it->second].push_back(i);
        ++in_degree[i];
      }
    }
  }

  std::vector<size_t> queue;
  for (size_t i = 0; i < pass_count; ++i) {
    if (in_degree[i] == 0) {
      queue.push_back(i);
    }
  }

  m_ExecutionOrder.clear();
  m_ExecutionOrder.reserve(pass_count);

  size_t front = 0;
  while (front < queue.size()) {
    size_t current = queue[front++];
    m_ExecutionOrder.push_back(current);

    for (size_t neighbor : adj[current]) {
      if (--in_degree[neighbor] == 0) {
        queue.push_back(neighbor);
      }
    }
  }

  if (m_ExecutionOrder.size() != pass_count) {
    throw std::runtime_error(
        "[RenderGraph] Cycle detected in render graph dependencies");
  }
}

void RenderGraph::createResources(RHIDevice& device)
{
  for (auto& [name, resource] : m_Resources) {
    if (name == BACKBUFFER) {
      continue;
    }
    if (resource.Target != nullptr) {
      continue;
    }

    RenderTargetDesc rt_desc;
    rt_desc.Width = resource.Desc.Width;
    rt_desc.Height = resource.Desc.Height;
    rt_desc.ColorFormat = resource.Desc.ColorFormat;
    rt_desc.DepthFormat = resource.Desc.DepthFormat;
    rt_desc.HasDepth = resource.Desc.HasDepth;
    resource.Target = device.CreateRenderTarget(rt_desc);
  }
}

void RenderGraph::buildRenderPassInfos()
{
  for (auto& pass : m_Passes) {
    RenderPassInfo info {};
    info.ColorAttachment = pass.Desc.ColorAttachment;

    bool writes_backbuffer = false;
    for (const auto& output : pass.Desc.Outputs) {
      if (output == BACKBUFFER) {
        writes_backbuffer = true;
        break;
      }
    }

    if (writes_backbuffer) {
      info.RenderTarget = nullptr;
    } else if (!pass.Desc.Outputs.empty()) {
      auto it = m_Resources.find(pass.Desc.Outputs[0]);
      if (it != m_Resources.end() && it->second.Target != nullptr) {
        info.RenderTarget = it->second.Target.get();
        info.Width = it->second.Target->GetWidth();
        info.Height = it->second.Target->GetHeight();
      }
    }

    if (pass.Desc.UseDepth) {
      pass.ResolvedDepthStencil = pass.Desc.DepthStencil;
      info.DepthStencilAttachment = &pass.ResolvedDepthStencil;
    }

    pass.ResolvedInfo = info;
  }
}
