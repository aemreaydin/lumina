#include <algorithm>
#include <cstddef>
#include <memory>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Renderer/RHI/RHISampler.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "Core/Application.hpp"
#include "Core/Logger.hpp"
#include "Renderer/RHI/RHIBuffer.hpp"
#include "Renderer/RHI/RHIDescriptorSet.hpp"
#include "Renderer/RHI/RHIDevice.hpp"
#include "Renderer/RHI/RHIPipeline.hpp"
#include "Renderer/RHI/RHIShaderModule.hpp"
#include "Renderer/RHI/RHITexture.hpp"
#include "Renderer/RHI/RHIVertexLayout.hpp"
#include "Renderer/ShaderCompiler.hpp"

struct Vertex
{
  glm::vec3 Position;
  glm::vec2 TexCoord;
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
        {1, DescriptorType::CombinedImageSampler, ShaderStage::Fragment, 1},
    };
    m_DescriptorSetLayout = GetDevice().CreateDescriptorSetLayout(layout_desc);

    // Create pipeline layout
    PipelineLayoutDesc pipeline_layout_desc {};
    pipeline_layout_desc.SetLayouts = {m_DescriptorSetLayout};
    m_PipelineLayout = GetDevice().CreatePipelineLayout(pipeline_layout_desc);

    // Compile shaders
    const auto shader_sources =
        ShaderCompiler::Compile("shaders/triangle.slang");

    const auto& vertex_spirv = shader_sources.at(ShaderType::Vertex);
    ShaderModuleDesc vertex_desc {};
    vertex_desc.Stage = ShaderStage::Vertex;
    vertex_desc.SPIRVCode = vertex_spirv;
    vertex_desc.EntryPoint = "vertexMain";
    vertex_desc.SetLayouts = {m_DescriptorSetLayout};
    m_VertexShader = GetDevice().CreateShaderModule(vertex_desc);

    const auto& fragment_spirv = shader_sources.at(ShaderType::Fragment);
    ShaderModuleDesc fragment_desc {};
    fragment_desc.Stage = ShaderStage::Fragment;
    fragment_desc.SPIRVCode = fragment_spirv;
    fragment_desc.EntryPoint = "fragmentMain";
    fragment_desc.SetLayouts = {m_DescriptorSetLayout};
    m_FragmentShader = GetDevice().CreateShaderModule(fragment_desc);

    m_Vertices = {{
        {.Position = {-0.5F, 0.5F, 0.0F}, .TexCoord = {0.0F, 0.0F}},
        {.Position = {-0.5F, -0.5F, 0.0F}, .TexCoord = {0.0F, 1.0F}},
        {.Position = {0.5F, -0.5F, 0.0F}, .TexCoord = {1.0F, 1.0F}},
        {.Position = {0.5F, 0.5F, 0.0F}, .TexCoord = {1.0F, 0.0F}},
    }};
    m_Indices = {0, 1, 2, 0, 2, 3};

    int width = 0;
    int height = 0;
    int num_channels = 0;
    const auto* image = stbi_load("assets/brick_wall_base.jpg",
                                  &width,
                                  &height,
                                  &num_channels,
                                  STBI_rgb_alpha);
    Logger::Info("Image Loaded - Width: {}, Height: {}, Num Channels: {}",
                 width,
                 height,
                 num_channels);

    TextureDesc tex_desc {};
    tex_desc.Format = TextureFormat::RGBA8Srgb;
    tex_desc.Usage = TextureUsage::Sampled;
    tex_desc.MipLevels = 1;
    tex_desc.Width = static_cast<uint32_t>(width);
    tex_desc.Height = static_cast<uint32_t>(height);
    m_Texture = GetDevice().CreateTexture(tex_desc);
    m_Texture->Upload(image, static_cast<size_t>(width * height) * 4);

    const SamplerDesc sampler_desc {};
    m_Sampler = GetDevice().CreateSampler(sampler_desc);

    BufferDesc vertex_buffer_desc {};
    vertex_buffer_desc.Size = sizeof(Vertex) * m_Vertices.size();
    vertex_buffer_desc.Usage = BufferUsage::Vertex;
    vertex_buffer_desc.CPUVisible = true;
    BufferDesc index_buffer_desc {};
    index_buffer_desc.Size = sizeof(uint32_t) * m_Indices.size();
    index_buffer_desc.Usage = BufferUsage::Index;
    index_buffer_desc.CPUVisible = true;

    m_VertexBuffer = GetDevice().CreateBuffer(vertex_buffer_desc);
    m_VertexBuffer->Upload(
        m_Vertices.data(), sizeof(Vertex) * m_Vertices.size(), 0);
    m_IndexBuffer = GetDevice().CreateBuffer(index_buffer_desc);
    m_IndexBuffer->Upload(
        m_Indices.data(), sizeof(uint32_t) * m_Indices.size(), 0);

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
    m_DescriptorSet->WriteCombinedImageSampler(
        1, m_Texture.get(), m_Sampler.get());

    // Set up vertex layout
    m_VertexLayout.Stride = sizeof(Vertex);
    m_VertexLayout.Attributes = {
        {0, VertexFormat::Float3, offsetof(Vertex, Position)},
        {1, VertexFormat::Float2, offsetof(Vertex, TexCoord)},
    };

    Logger::Info("SandboxApp::OnInit - Triangle resources created");
  }

  void OnRender(float delta_time) override
  {
    auto& device = GetDevice();

    m_Angle += delta_time;

    Transforms transforms {};
    transforms.MVP =
        glm::rotate(glm::mat4(1.0F), m_Angle, glm::vec3(0.0F, 0.0F, 1.0F));

    // Upload MVP to uniform buffer
    m_UniformBuffer->Upload(&transforms, sizeof(Transforms), 0);

    device.BindShaders(m_VertexShader.get(), m_FragmentShader.get());
    device.BindVertexBuffer(*m_VertexBuffer, 0);
    device.SetVertexInput(m_VertexLayout);
    device.BindIndexBuffer(*m_IndexBuffer);
    device.SetPrimitiveTopology(PrimitiveTopology::TriangleList);
    device.BindDescriptorSet(0, *m_DescriptorSet, *m_PipelineLayout);
    device.DrawIndexed(static_cast<uint32_t>(m_Indices.size()), 1, 0, nullptr);
  }

  void OnDestroy() override
  {
    Logger::Info("SandboxApp::OnDestroy - Cleaning up resources");
    m_DescriptorSet.reset();
    m_Texture.reset();
    m_Sampler.reset();
    m_UniformBuffer.reset();
    m_VertexBuffer.reset();
    m_IndexBuffer.reset();
    m_FragmentShader.reset();
    m_VertexShader.reset();
    m_PipelineLayout.reset();
    m_DescriptorSetLayout.reset();
  }

private:
  std::unique_ptr<RHIBuffer> m_VertexBuffer;
  std::unique_ptr<RHIBuffer> m_IndexBuffer;
  std::unique_ptr<RHIBuffer> m_UniformBuffer;
  std::unique_ptr<RHIShaderModule> m_VertexShader;
  std::unique_ptr<RHIShaderModule> m_FragmentShader;
  std::shared_ptr<RHIDescriptorSetLayout> m_DescriptorSetLayout;
  std::shared_ptr<RHIPipelineLayout> m_PipelineLayout;
  std::unique_ptr<RHIDescriptorSet> m_DescriptorSet;
  std::unique_ptr<RHITexture> m_Texture;
  std::unique_ptr<RHISampler> m_Sampler;
  std::vector<Vertex> m_Vertices;
  std::vector<uint32_t> m_Indices;
  VertexInputLayout m_VertexLayout;
  float m_Angle = 0.0F;
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
