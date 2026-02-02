#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>

#include <linalg/vec.hpp>
#include <linalg/mat4.hpp>

#include "Renderer/RHI/RHISampler.hpp"
#include "Renderer/RHI/RHISwapchain.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include "Core/Application.hpp"
#include "Core/Input.hpp"
#include "Core/Logger.hpp"
#include "Core/Window.hpp"
#include "Renderer/Camera.hpp"
#include "Renderer/CameraController.hpp"
#include "Renderer/RHI/RHIBuffer.hpp"
#include "Renderer/RHI/RHICommandBuffer.hpp"
#include "Renderer/RHI/RHIDescriptorSet.hpp"
#include "Renderer/RHI/RHIDevice.hpp"
#include "Renderer/RHI/RHIPipeline.hpp"
#include "Renderer/RHI/RHIShaderModule.hpp"
#include "Renderer/RHI/RHITexture.hpp"
#include "Renderer/RHI/RHIVertexLayout.hpp"
#include "Renderer/ShaderCompiler.hpp"

struct Vertex
{
  linalg::Vec3 Position;
  linalg::Vec2 TexCoord;
};

struct Transforms
{
  linalg::Mat4 MVP;
};

struct ModelData
{
  std::vector<Vertex> Vertices;
  std::vector<uint32_t> Indices;
};

static auto LoadModel(const std::string& path) -> ModelData
{
  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;
  std::string warn;
  std::string err;

  if (!tinyobj::LoadObj(
          &attrib, &shapes, &materials, &warn, &err, path.c_str()))
  {
    Logger::Error("Failed to load model: {} {}", warn, err);
    return {};
  }

  if (!warn.empty()) {
    Logger::Warn("Model load warning: {}", warn);
  }

  ModelData model;
  std::unordered_map<size_t, uint32_t> unique_vertices;

  for (const auto& shape : shapes) {
    for (const auto& index : shape.mesh.indices) {
      Vertex vertex {};

      vertex.Position = {
          attrib.vertices[(3 * static_cast<size_t>(index.vertex_index)) + 0],
          attrib.vertices[(3 * static_cast<size_t>(index.vertex_index)) + 1],
          attrib.vertices[(3 * static_cast<size_t>(index.vertex_index)) + 2],
      };

      if (index.texcoord_index >= 0) {
        vertex.TexCoord = {
            attrib
                .texcoords[(2 * static_cast<size_t>(index.texcoord_index)) + 0],
            1.0F
                - attrib
                      .texcoords[(2 * static_cast<size_t>(index.texcoord_index))
                                 + 1],
        };
      }

      const size_t hash = std::hash<float> {}(vertex.Position.x())
          ^ (std::hash<float> {}(vertex.Position.y()) << 1)
          ^ (std::hash<float> {}(vertex.Position.z()) << 2)
          ^ (std::hash<float> {}(vertex.TexCoord.x()) << 3)
          ^ (std::hash<float> {}(vertex.TexCoord.y()) << 4);

      if (!unique_vertices.contains(hash)) {
        unique_vertices[hash] = static_cast<uint32_t>(model.Vertices.size());
        model.Vertices.push_back(vertex);
      }

      model.Indices.push_back(unique_vertices[hash]);
    }
  }

  Logger::Info("Loaded model: {} vertices, {} indices",
               model.Vertices.size(),
               model.Indices.size());

  return model;
}

class DepthApp : public Application
{
public:
  DepthApp() = default;

protected:
  void OnInit() override
  {
    Logger::Info("DepthApp::OnInit - Loading model resources");

    DescriptorSetLayoutDesc layout_desc {};
    layout_desc.Bindings = {
        {0, DescriptorType::UniformBuffer, ShaderStage::Vertex, 1},
        {1, DescriptorType::CombinedImageSampler, ShaderStage::Fragment, 1},
    };
    m_DescriptorSetLayout = GetDevice().CreateDescriptorSetLayout(layout_desc);

    m_PipelineLayout =
        GetDevice().CreatePipelineLayout({m_DescriptorSetLayout});

    const auto shader_sources = ShaderCompiler::Compile("shaders/depth.slang");

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

    auto model = LoadModel("assets/volleyball/volleyball.obj");
    m_Vertices = std::move(model.Vertices);
    m_Indices = std::move(model.Indices);

    int width = 0;
    int height = 0;
    int num_channels = 0;
    const auto* image =
        stbi_load("assets/volleyball/textures/volleyball_Albedo.png",
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

    BufferDesc uniform_desc {};
    uniform_desc.Size = sizeof(Transforms);
    uniform_desc.Usage = BufferUsage::Uniform;
    uniform_desc.CPUVisible = true;
    m_UniformBuffer = GetDevice().CreateBuffer(uniform_desc);

    m_DescriptorSet = GetDevice().CreateDescriptorSet(m_DescriptorSetLayout);
    m_DescriptorSet->WriteBuffer(
        0, m_UniformBuffer.get(), 0, sizeof(Transforms));
    m_DescriptorSet->WriteCombinedImageSampler(
        1, m_Texture.get(), m_Sampler.get());

    m_VertexLayout.Stride = sizeof(Vertex);
    m_VertexLayout.Attributes = {
        {0, VertexFormat::Float3, offsetof(Vertex, Position)},
        {1, VertexFormat::Float2, offsetof(Vertex, TexCoord)},
    };

    // Setup camera
    m_Camera.SetPerspective(45.0F, 16.0F / 9.0F, 0.01F, 100.0F);
    m_Camera.SetPosition(linalg::Vec3{20.0F, 20.0F, 10.0F});
    m_Camera.SetTarget(linalg::Vec3{0.0F, 0.0F, 0.0F});

    // Create both camera controllers
    m_OrbitController = std::make_unique<OrbitCameraController>(&m_Camera);
    m_OrbitController->SetTarget(linalg::Vec3{0.0F, 0.0F, 0.0F});
    m_OrbitController->SetDistance(20.0F);
    m_OrbitController->SetDistanceLimits(5.0F, 50.0F);

    m_FPSController = std::make_unique<FPSCameraController>(&m_Camera);
    m_FPSController->SetMoveSpeed(10.0F);

    // Start with orbit camera
    m_ActiveController = m_OrbitController.get();

    Logger::Info("DepthApp::OnInit - Model resources created");
    Logger::Info("Controls: 1=Orbit camera, 2=FPS camera, ESC=Exit");
  }

  void OnUpdate(float delta_time) override
  {
    // ESC to exit
    if (Input::IsKeyPressed(KeyCode::Escape)) {
      GetWindow().RequestClose();
      return;
    }

    // 1 = Orbit camera
    if (Input::IsKeyPressed(KeyCode::Num1)) {
      m_ActiveController = m_OrbitController.get();
      Input::SetMouseCaptured(/*captured=*/false);
      Logger::Info("Switched to Orbit camera");
    }

    // 2 = FPS camera
    if (Input::IsKeyPressed(KeyCode::Num2)) {
      m_ActiveController = m_FPSController.get();
      Logger::Info("Switched to FPS camera (hold right-click to look)");
    }

    m_ActiveController->Update(delta_time);

    // Update aspect ratio on window resize
    auto* swapchain = GetDevice().GetSwapchain();
    const float aspect = static_cast<float>(swapchain->GetWidth())
        / static_cast<float>(swapchain->GetHeight());
    m_Camera.SetAspectRatio(aspect);
  }

  void OnRender(float /*delta_time*/) override
  {
    auto& device = GetDevice();
    auto* cmd = device.GetCurrentCommandBuffer();

    const linalg::Mat4 model = linalg::Mat4::identity();

    Transforms transforms {};
    transforms.MVP = m_Camera.GetViewProjectionMatrix() * model;

    m_UniformBuffer->Upload(&transforms, sizeof(Transforms), 0);

    cmd->BindShaders(m_VertexShader.get(), m_FragmentShader.get());
    cmd->BindVertexBuffer(*m_VertexBuffer, 0);
    cmd->SetVertexInput(m_VertexLayout);
    cmd->BindIndexBuffer(*m_IndexBuffer);
    cmd->SetPrimitiveTopology(PrimitiveTopology::TriangleList);
    cmd->BindDescriptorSet(0, *m_DescriptorSet, *m_PipelineLayout);
    cmd->DrawIndexed(static_cast<uint32_t>(m_Indices.size()), 1, 0, 0, 0);
  }

  void OnDestroy() override
  {
    Logger::Info("DepthApp::OnDestroy - Cleaning up resources");
    m_ActiveController = nullptr;
    m_OrbitController.reset();
    m_FPSController.reset();
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

  Camera m_Camera;
  std::unique_ptr<OrbitCameraController> m_OrbitController;
  std::unique_ptr<FPSCameraController> m_FPSController;
  CameraController* m_ActiveController {nullptr};
};

auto CreateApplication() -> std::unique_ptr<Application>
{
  return std::make_unique<DepthApp>();
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
