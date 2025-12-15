#include <algorithm>
#include <array>
#include <memory>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Core/Application.hpp"
#include "Core/Logger.hpp"
#include "Renderer/RHI/RHIBuffer.hpp"
#include "Renderer/RHI/RHIDescriptorSet.hpp"
#include "Renderer/RHI/RHIDevice.hpp"
#include "Renderer/RHI/RHIPipeline.hpp"
#include "Renderer/RHI/RHIShaderModule.hpp"
#include "Renderer/RHI/RHIVertexLayout.hpp"
#include "Renderer/ShaderCompiler.hpp"

struct Vertex
{
  glm::vec3 Position;
  glm::vec3 Color;
};

struct Transforms
{
  glm::mat4 MVP;
};

class SandboxApp : public Application
{
public:
  SandboxApp() = default;

protected:
  void OnInit() override
  {
    Logger::Info("SandboxApp::OnInit - Creating triangle resources");

    // Create descriptor set layout first (needed for shader creation with
    // VK_EXT_shader_object)
    DescriptorSetLayoutDesc layout_desc {};
    layout_desc.Bindings = {
        {0, DescriptorType::UniformBuffer, ShaderStage::Vertex, 1},
    };
    m_DescriptorSetLayout = GetDevice().CreateDescriptorSetLayout(layout_desc);

    // Create pipeline layout
    PipelineLayoutDesc pipeline_layout_desc {};
    pipeline_layout_desc.SetLayouts = {m_DescriptorSetLayout};
    m_PipelineLayout = GetDevice().CreatePipelineLayout(pipeline_layout_desc);

    // Compile shaders
    const auto shader_sources =
        ShaderCompiler::Compile("shaders/triangle.slang");

    // Create vertex shader (with descriptor set layouts for
    // VK_EXT_shader_object)
    const auto& vertex_spirv = shader_sources.at(ShaderType::Vertex);
    ShaderModuleDesc vertex_desc {};
    vertex_desc.Stage = ShaderStage::Vertex;
    vertex_desc.SPIRVCode = vertex_spirv;
    vertex_desc.EntryPoint = "vertexMain";
    vertex_desc.SetLayouts = {m_DescriptorSetLayout};
    m_VertexShader = GetDevice().CreateShaderModule(vertex_desc);

    // Create fragment shader (with descriptor set layouts for
    // VK_EXT_shader_object)
    const auto& fragment_spirv = shader_sources.at(ShaderType::Fragment);
    ShaderModuleDesc fragment_desc {};
    fragment_desc.Stage = ShaderStage::Fragment;
    fragment_desc.SPIRVCode = fragment_spirv;
    fragment_desc.EntryPoint = "fragmentMain";
    fragment_desc.SetLayouts = {m_DescriptorSetLayout};
    m_FragmentShader = GetDevice().CreateShaderModule(fragment_desc);

    // Triangle vertices (CCW winding)
    constexpr std::array<Vertex, 3> VERTICES = {{
        {.Position = {0.0F, 0.5F, 0.0F},
         .Color = {1.0F, 0.0F, 0.0F}},  // Top - Red
        {.Position = {-0.5F, -0.5F, 0.0F},
         .Color = {0.0F, 1.0F, 0.0F}},  // Bottom Left - Green
        {.Position = {0.5F, -0.5F, 0.0F},
         .Color = {0.0F, 0.0F, 1.0F}},  // Bottom Right - Blue
    }};

    // Create vertex buffer
    BufferDesc buffer_desc {};
    buffer_desc.Size = sizeof(VERTICES);
    buffer_desc.Usage = BufferUsage::Vertex;
    buffer_desc.CPUVisible = true;

    m_VertexBuffer = GetDevice().CreateBuffer(buffer_desc);
    m_VertexBuffer->Upload(VERTICES.data(), sizeof(VERTICES), 0);

    // Create uniform buffer for MVP matrix
    BufferDesc uniform_desc {};
    uniform_desc.Size = sizeof(Transforms);
    uniform_desc.Usage = BufferUsage::Uniform;
    uniform_desc.CPUVisible = true;
    m_UniformBuffer = GetDevice().CreateBuffer(uniform_desc);

    // Create descriptor set and write uniform buffer
    m_DescriptorSet = GetDevice().CreateDescriptorSet(m_DescriptorSetLayout);
    m_DescriptorSet->WriteBuffer(
        0, m_UniformBuffer.get(), 0, sizeof(Transforms));

    // Set up vertex layout
    m_VertexLayout.Stride = sizeof(Vertex);
    m_VertexLayout.Attributes = {
        {0, VertexFormat::Float3, offsetof(Vertex, Position)},
        {1, VertexFormat::Float3, offsetof(Vertex, Color)},
    };

    Logger::Info("SandboxApp::OnInit - Triangle resources created");
  }

  void OnRender(float delta_time) override
  {
    auto& device = GetDevice();

    // Accumulate time and calculate rotation
    // Clamp delta to prevent jarring jumps during resize/stutter
    constexpr float MAX_DELTA = 0.1F;
    m_TotalTime += std::min(delta_time, MAX_DELTA);
    const float angle = m_TotalTime;  // Radians per second

    // Calculate MVP matrix (identity model, identity view, identity projection)
    // Just apply rotation around Z axis
    Transforms transforms {};
    transforms.MVP =
        glm::rotate(glm::mat4(1.0F), angle, glm::vec3(0.0F, 0.0F, 1.0F));

    // Upload MVP to uniform buffer
    m_UniformBuffer->Upload(&transforms, sizeof(Transforms), 0);

    device.BindShaders(m_VertexShader.get(), m_FragmentShader.get());
    device.BindVertexBuffer(*m_VertexBuffer, 0);
    device.SetVertexInput(m_VertexLayout);
    device.SetPrimitiveTopology(PrimitiveTopology::TriangleList);
    device.BindDescriptorSet(0, *m_DescriptorSet, *m_PipelineLayout);
    device.Draw(3, 1, 0, 0);
  }

  void OnDestroy() override
  {
    Logger::Info("SandboxApp::OnDestroy - Cleaning up resources");
    m_DescriptorSet.reset();
    m_UniformBuffer.reset();
    m_VertexBuffer.reset();
    m_FragmentShader.reset();
    m_VertexShader.reset();
    m_PipelineLayout.reset();
    m_DescriptorSetLayout.reset();
  }

private:
  std::unique_ptr<RHIBuffer> m_VertexBuffer;
  std::unique_ptr<RHIBuffer> m_UniformBuffer;
  std::unique_ptr<RHIShaderModule> m_VertexShader;
  std::unique_ptr<RHIShaderModule> m_FragmentShader;
  std::shared_ptr<RHIDescriptorSetLayout> m_DescriptorSetLayout;
  std::shared_ptr<RHIPipelineLayout> m_PipelineLayout;
  std::unique_ptr<RHIDescriptorSet> m_DescriptorSet;
  VertexInputLayout m_VertexLayout;
  float m_TotalTime = 0.0F;
};

auto CreateApplication() -> std::unique_ptr<Application>
{
  return std::make_unique<SandboxApp>();
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
