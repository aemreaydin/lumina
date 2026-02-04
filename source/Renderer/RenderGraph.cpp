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
  // Check MRT attachment map first
  auto map_it = m_AttachmentMap.find(name);
  if (map_it != m_AttachmentMap.end()) {
    if (map_it->second.IsDepth) {
      return map_it->second.SharedTarget->GetDepthTexture();
    }
    return map_it->second.SharedTarget->GetColorTexture(
        map_it->second.AttachmentIndex);
  }

  // Fallback to single-resource path
  auto it = m_Resources.find(name);
  if (it == m_Resources.end() || it->second.Target == nullptr) {
    return nullptr;
  }
  return it->second.Target->GetColorTexture(0);
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
  m_AttachmentMap.clear();
  m_SharedTargets.clear();
  createResources(device);
  buildRenderPassInfos();
}

void RenderGraph::Clear()
{
  m_Resources.clear();
  m_Passes.clear();
  m_ExecutionOrder.clear();
  m_AttachmentMap.clear();
  m_SharedTargets.clear();
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
  m_AttachmentMap.clear();
  m_SharedTargets.clear();

  for (auto& pass : m_Passes) {
    std::vector<std::string> color_outputs;
    std::string depth_output;

    for (const auto& output : pass.Desc.Outputs) {
      if (output == BACKBUFFER) {
        continue;
      }
      auto it = m_Resources.find(output);
      if (it == m_Resources.end()) {
        continue;
      }

      if (it->second.Desc.IsDepth) {
        depth_output = output;
      } else {
        color_outputs.push_back(output);
      }
    }

    // MRT case: multiple color outputs or color + depth from one pass
    if (color_outputs.size() > 1
        || (!color_outputs.empty() && !depth_output.empty()))
    {
      RenderTargetDesc rt_desc;
      rt_desc.Width = m_Resources[color_outputs[0]].Desc.Width;
      rt_desc.Height = m_Resources[color_outputs[0]].Desc.Height;
      rt_desc.ColorFormats.clear();

      for (const auto& name : color_outputs) {
        rt_desc.ColorFormats.push_back(m_Resources[name].Desc.ColorFormat);
      }

      if (!depth_output.empty()) {
        rt_desc.DepthFormat = m_Resources[depth_output].Desc.DepthFormat;
        rt_desc.HasDepth = true;
      } else {
        rt_desc.HasDepth = false;
      }

      auto shared_rt = device.CreateRenderTarget(rt_desc);
      auto* rt_ptr = shared_rt.get();

      for (size_t i = 0; i < color_outputs.size(); ++i) {
        m_AttachmentMap[color_outputs[i]] = {
            .SharedTarget = rt_ptr,
            .AttachmentIndex = i,
            .IsDepth = false,
        };
      }
      if (!depth_output.empty()) {
        m_AttachmentMap[depth_output] = {
            .SharedTarget = rt_ptr,
            .AttachmentIndex = 0,
            .IsDepth = true,
        };
      }

      m_SharedTargets.push_back(std::move(shared_rt));
      continue;  // Skip single-output path for passes already handled
    }

    // Single output -- existing path
    for (const auto& output : pass.Desc.Outputs) {
      if (output == BACKBUFFER) {
        continue;
      }
      auto& resource = m_Resources[output];
      if (resource.Target != nullptr) {
        continue;
      }

      RenderTargetDesc rt_desc;
      rt_desc.Width = resource.Desc.Width;
      rt_desc.Height = resource.Desc.Height;
      rt_desc.ColorFormats = {resource.Desc.ColorFormat};
      rt_desc.DepthFormat = resource.Desc.DepthFormat;
      rt_desc.HasDepth = resource.Desc.HasDepth;
      resource.Target = device.CreateRenderTarget(rt_desc);
    }
  }
}

void RenderGraph::buildRenderPassInfos()
{
  for (auto& pass : m_Passes) {
    RenderPassInfo info {};
    info.ColorAttachments = pass.Desc.ColorAttachments;
    info.ColorAttachmentCount = pass.Desc.ColorAttachmentCount;

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
      // Check MRT attachment map first
      const auto& first_output = pass.Desc.Outputs[0];
      auto map_it = m_AttachmentMap.find(first_output);
      if (map_it != m_AttachmentMap.end()) {
        info.RenderTarget = map_it->second.SharedTarget;
        info.Width = info.RenderTarget->GetWidth();
        info.Height = info.RenderTarget->GetHeight();
      } else {
        // Single output path
        auto it = m_Resources.find(first_output);
        if (it != m_Resources.end() && it->second.Target != nullptr) {
          info.RenderTarget = it->second.Target.get();
          info.Width = it->second.Target->GetWidth();
          info.Height = it->second.Target->GetHeight();
        }
      }
    }

    if (pass.Desc.UseDepth) {
      pass.ResolvedDepthStencil = pass.Desc.DepthStencil;
      info.DepthStencilAttachment = &pass.ResolvedDepthStencil;
    }

    pass.ResolvedInfo = info;
  }
}
