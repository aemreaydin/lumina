#include <cstddef>
#include <memory>

#include <linalg/vec.hpp>

#include "Core/Application.hpp"
#include "Core/Logger.hpp"
#include "Renderer/RHI/RHIBuffer.hpp"
#include "Renderer/RHI/RHICommandBuffer.hpp"
#include "Renderer/RHI/RHIDescriptorSet.hpp"
#include "Renderer/RHI/RHIDevice.hpp"
#include "Renderer/RHI/RHIPipeline.hpp"
#include "Renderer/RHI/RHIShaderModule.hpp"
#include "Renderer/RHI/RHIVertexLayout.hpp"
#include "Renderer/ShaderCompiler.hpp"

struct Vertex
{
  linalg::Vec3 Position;
  linalg::Vec3 Color;
};

class TriangleApp : public Application
{
public:
  TriangleApp() = default;

protected:
  void OnInit() override
  {
    Logger::Info("TriangleApp::OnInit - Creating triangle resources");

    const DescriptorSetLayoutDesc layout_desc {};
    m_DescriptorSetLayout = GetDevice().CreateDescriptorSetLayout(layout_desc);

    m_PipelineLayout =
        GetDevice().CreatePipelineLayout({m_DescriptorSetLayout});

    const auto shader_sources =
        ShaderCompiler::Compile("shaders/triangle.slang");

    const auto& vertex_spirv = shader_sources.Sources.at(ShaderType::Vertex);
    ShaderModuleDesc vertex_desc {};
    vertex_desc.Stage = ShaderStage::Vertex;
    vertex_desc.SPIRVCode = vertex_spirv;
    vertex_desc.EntryPoint = "vertexMain";
    vertex_desc.SetLayouts = {m_DescriptorSetLayout};
    m_VertexShader = GetDevice().CreateShaderModule(vertex_desc);

    const auto& fragment_spirv =
        shader_sources.Sources.at(ShaderType::Fragment);
    ShaderModuleDesc fragment_desc {};
    fragment_desc.Stage = ShaderStage::Fragment;
    fragment_desc.SPIRVCode = fragment_spirv;
    fragment_desc.EntryPoint = "fragmentMain";
    fragment_desc.SetLayouts = {m_DescriptorSetLayout};
    m_FragmentShader = GetDevice().CreateShaderModule(fragment_desc);

    m_Vertices = {{
        {.Position = {0.0F, 0.5F, 0.0F}, .Color = {1.0F, 0.0F, 0.0F}},
        {.Position = {-0.5F, -0.5F, 0.0F}, .Color = {0.0F, 1.0F, 0.0F}},
        {.Position = {0.5F, -0.5F, 0.0F}, .Color = {0.0F, 0.0F, 1.0F}},
    }};

    BufferDesc vertex_buffer_desc {};
    vertex_buffer_desc.Size = sizeof(Vertex) * m_Vertices.size();
    vertex_buffer_desc.Usage = BufferUsage::Vertex;
    vertex_buffer_desc.CPUVisible = true;

    m_VertexBuffer = GetDevice().CreateBuffer(vertex_buffer_desc);
    m_VertexBuffer->Upload(
        m_Vertices.data(), sizeof(Vertex) * m_Vertices.size(), 0);

    m_VertexLayout.Stride = sizeof(Vertex);
    m_VertexLayout.Attributes = {
        {0, VertexFormat::Float3, offsetof(Vertex, Position)},
        {1, VertexFormat::Float3, offsetof(Vertex, Color)},
    };

    Logger::Info("TriangleApp::OnInit - Triangle resources created");
  }

  void OnRender(float /*delta_time*/) override
  {
    auto& device = GetDevice();
    auto* cmd = device.GetCurrentCommandBuffer();

    cmd->BindShaders(m_VertexShader.get(), m_FragmentShader.get());
    cmd->BindVertexBuffer(*m_VertexBuffer, 0);
    cmd->SetVertexInput(m_VertexLayout);
    cmd->SetPrimitiveTopology(PrimitiveTopology::TriangleList);
    cmd->Draw(static_cast<uint32_t>(m_Vertices.size()), 1, 0, 0);
  }

  void OnDestroy() override
  {
    Logger::Info("TriangleApp::OnDestroy - Cleaning up resources");
    m_VertexBuffer.reset();
    m_FragmentShader.reset();
    m_VertexShader.reset();
    m_PipelineLayout.reset();
    m_DescriptorSetLayout.reset();
  }

private:
  std::unique_ptr<RHIBuffer> m_VertexBuffer;
  std::unique_ptr<RHIShaderModule> m_VertexShader;
  std::unique_ptr<RHIShaderModule> m_FragmentShader;
  std::shared_ptr<RHIDescriptorSetLayout> m_DescriptorSetLayout;
  std::shared_ptr<RHIPipelineLayout> m_PipelineLayout;
  std::vector<Vertex> m_Vertices;
  VertexInputLayout m_VertexLayout;
};

auto CreateApplication() -> std::unique_ptr<Application>
{
  return std::make_unique<TriangleApp>();
}

auto main() -> int
{
  const auto app = CreateApplication();
  app->Init();
  app->Run();
  app->Destroy();

  Logger::Info("Application shutting down");
  return 0;
}
