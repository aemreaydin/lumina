#include <memory>

#include <linalg/vec.hpp>

#include "Core/Application.hpp"
#include "Core/Input.hpp"
#include "Core/Logger.hpp"
#include "Core/Window.hpp"
#include "Renderer/Asset/AssetManager.hpp"
#include "Renderer/Camera.hpp"
#include "Renderer/CameraController.hpp"
#include "Renderer/Model/Model.hpp"
#include "Renderer/RHI/RHICommandBuffer.hpp"
#include "Renderer/RHI/RHIDescriptorSet.hpp"
#include "Renderer/RHI/RHIDevice.hpp"
#include "Renderer/RHI/RHISwapchain.hpp"
#include "Renderer/RHI/RHISampler.hpp"
#include "Renderer/RenderGraph.hpp"
#include "Renderer/Scene/Scene.hpp"
#include "Renderer/Scene/SceneNode.hpp"
#include "Renderer/Scene/SceneRenderer.hpp"
#include "Renderer/ShaderCompiler.hpp"
#include "UI/RHIImGui.hpp"

class RenderGraphDemoApp : public Application
{
public:
  RenderGraphDemoApp() = default;

protected:
  void OnInit() override
  {
    Logger::Info("RenderGraphDemoApp::OnInit - Setting up render graph demo");

    m_AssetManager = std::make_unique<AssetManager>(GetDevice());
    m_SceneRenderer =
        std::make_unique<SceneRenderer>(GetDevice(), GetRendererConfig().API);
    m_AssetManager->SetMaterialDescriptorSetLayout(
        m_SceneRenderer->GetSetLayout("material"));

    m_Scene = std::make_unique<Scene>("RenderGraph Demo Scene");

    auto lion_head = m_AssetManager->LoadModel("lion_head/lion_head_4k.obj");
    if (!lion_head) {
      Logger::Error("Failed to load model!");
      throw std::runtime_error("Failed to load model");
    }
    auto coffee_table =
        m_AssetManager->LoadModel("coffee_table/gothic_coffee_table_4k.obj");
    if (!coffee_table) {
      Logger::Error("Failed to load model!");
      throw std::runtime_error("Failed to load model");
    }
    auto chair =
        m_AssetManager->LoadModel("chair/mid_century_lounge_chair_4k.obj");
    if (!chair) {
      Logger::Error("Failed to load model!");
      throw std::runtime_error("Failed to load model");
    }

    auto* node1 = m_Scene->CreateNode("Lion Head");
    node1->SetModel(lion_head);
    node1->SetPosition(linalg::Vec3 {0.0F, 0.0F, 0.0F});
    node1->SetScale(10.0F);

    auto* node2 = m_Scene->CreateNode("Coffee Table");
    node2->SetModel(coffee_table);
    node2->SetPosition(linalg::Vec3 {5.0F, 0.0F, 0.0F});
    node2->SetScale(6.0F);

    auto* node3 = m_Scene->CreateNode("Chair");
    node3->SetModel(chair);
    node3->SetPosition(linalg::Vec3 {-5.0F, 0.0F, 0.0F});
    node3->SetScale(3.0F);

    m_Camera.SetPerspective(45.0F, 16.0F / 9.0F, 0.01F, 1000.0F);
    m_Camera.SetPosition(linalg::Vec3 {0.0F, 15.0F, 5.0F});
    m_Camera.SetTarget(linalg::Vec3 {0.0F, 0.0F, 0.0F});

    m_FPSController = std::make_unique<FPSCameraController>(&m_Camera);

    GetImGui().SetCamera(m_Camera);

    // Compile fullscreen quad shader for the composite pass
    setupCompositeShader();

    // Build the render graph
    setupRenderGraph();

    Logger::Info("RenderGraph demo initialized with {} nodes",
                 m_Scene->GetNodeCount());
    Logger::Info("Controls: ESC=Exit, F1=Settings, F2=Scene Hierarchy");
  }

  void OnUpdate(float delta_time) override
  {
    if (Input::IsKeyPressed(KeyCode::Escape)) {
      GetWindow().RequestClose();
      return;
    }

    m_FPSController->Update(delta_time);

    auto* swapchain = GetDevice().GetSwapchain();
    const float aspect = static_cast<float>(swapchain->GetWidth())
        / static_cast<float>(swapchain->GetHeight());
    m_Camera.SetAspectRatio(aspect);

    m_Scene->UpdateTransforms();

    if (Input::IsMouseButtonPressed(MouseButton::Left)) {
      auto* swapchain = GetDevice().GetSwapchain();
      const auto mouse_pos = Input::GetMousePosition();
      const auto ray =
          m_Camera.ScreenPointToRay(mouse_pos.x(),
                                    mouse_pos.y(),
                                    static_cast<float>(swapchain->GetWidth()),
                                    static_cast<float>(swapchain->GetHeight()));
      GetImGui().SetSelectedNode(m_Scene->PickNode(ray));
    }

    if (swapchain->GetWidth() != m_LastWidth
        || swapchain->GetHeight() != m_LastHeight)
    {
      m_LastWidth = swapchain->GetWidth();
      m_LastHeight = swapchain->GetHeight();
      GetRenderGraph().Resize(GetDevice(), m_LastWidth, m_LastHeight);

      rebindSceneTexture();
    }
  }

  void OnDestroy() override
  {
    Logger::Info("RenderGraphDemoApp::OnDestroy - Cleaning up");
    m_FPSController.reset();
    m_CompositeDescriptorSet.reset();
    m_CompositePipelineLayout.reset();
    m_CompositeDescriptorSetLayout.reset();
    m_CompositeVS.reset();
    m_CompositeFS.reset();
    m_CompositeSampler.reset();
    m_SceneRenderer.reset();
    m_Scene.reset();
    m_AssetManager.reset();
  }

private:
  void setupCompositeShader()
  {
    DescriptorSetLayoutDesc layout_desc {};
    layout_desc.Bindings = {
        {0, DescriptorType::CombinedImageSampler, ShaderStage::Fragment, 1},
    };
    m_CompositeDescriptorSetLayout =
        GetDevice().CreateDescriptorSetLayout(layout_desc);
    m_CompositePipelineLayout =
        GetDevice().CreatePipelineLayout({m_CompositeDescriptorSetLayout});

    const auto shader_sources = ShaderCompiler::Compile(
        "shaders/fullscreen_quad.slang", GetRendererConfig().API);

    ShaderModuleDesc vertex_desc {};
    vertex_desc.Stage = ShaderStage::Vertex;
    vertex_desc.SPIRVCode = shader_sources.GetSPIRV(ShaderType::Vertex);
    vertex_desc.GLSLCode = shader_sources.GetGLSL(ShaderType::Vertex);
    vertex_desc.EntryPoint = "vertexMain";
    vertex_desc.SetLayouts = {m_CompositeDescriptorSetLayout};
    m_CompositeVS = GetDevice().CreateShaderModule(vertex_desc);

    ShaderModuleDesc fragment_desc {};
    fragment_desc.Stage = ShaderStage::Fragment;
    fragment_desc.SPIRVCode = shader_sources.GetSPIRV(ShaderType::Fragment);
    fragment_desc.GLSLCode = shader_sources.GetGLSL(ShaderType::Fragment);
    fragment_desc.EntryPoint = "fragmentMain";
    fragment_desc.SetLayouts = {m_CompositeDescriptorSetLayout};
    m_CompositeFS = GetDevice().CreateShaderModule(fragment_desc);

    SamplerDesc sampler_desc {};
    sampler_desc.MinFilter = Filter::Linear;
    sampler_desc.MagFilter = Filter::Linear;
    sampler_desc.MaxLod = 0.0F;
    m_CompositeSampler = GetDevice().CreateSampler(sampler_desc);

    m_CompositeDescriptorSet =
        GetDevice().CreateDescriptorSet(m_CompositeDescriptorSetLayout);
  }

  void setupRenderGraph()
  {
    auto* swapchain = GetDevice().GetSwapchain();
    m_LastWidth = swapchain->GetWidth();
    m_LastHeight = swapchain->GetHeight();

    auto& graph = GetRenderGraph();

    graph.AddResource({
        .Name = "SceneColor",
        .Width = m_LastWidth,
        .Height = m_LastHeight,
        .ColorFormat = TextureFormat::RGBA8Srgb,
        .HasDepth = true,
    });

    graph.AddPass({
        .Name = "ScenePass",
        .Outputs = {"SceneColor"},
        .ColorAttachment =
            {.ColorLoadOp = LoadOp::Clear,
             .ClearColor = {.R = 0.1F, .G = 0.1F, .B = 0.1F, .A = 1.0F}},
        .UseDepth = true,
        .DepthStencil =
            {.DepthLoadOp = LoadOp::Clear,
             .DepthStoreOp = StoreOp::DontCare,
             .ClearDepthStencil = {.Depth = 1.0F}},
        .Execute =
            [this](RHICommandBuffer& cmd)
        {
          m_SceneRenderer->SetWireframe(GetImGui().IsWireframe());
          m_SceneRenderer->BeginFrame(m_Camera);
          m_SceneRenderer->RenderScene(cmd, *m_Scene);
        },
    });

    graph.AddPass({
        .Name = "Composite",
        .Inputs = {"SceneColor"},
        .Outputs = {RenderGraph::BACKBUFFER},
        .ColorAttachment =
            {.ColorLoadOp = LoadOp::Clear,
             .ClearColor = {.R = 0.0F, .G = 1.0F, .B = 0.0F, .A = 1.0F}},
        .UseDepth = false,
        .Execute =
            [this](RHICommandBuffer& cmd)
        {
          cmd.SetPrimitiveTopology(PrimitiveTopology::TriangleList);
          cmd.BindShaders(m_CompositeVS.get(), m_CompositeFS.get());
          cmd.BindDescriptorSet(
              0, *m_CompositeDescriptorSet, *m_CompositePipelineLayout);
          cmd.Draw(3, 1, 0, 0);

          GetImGui().RenderPanels(*m_Scene);
          GetImGui().EndFrame();
        },
    });

    graph.Compile(GetDevice());

    rebindSceneTexture();
  }

  void rebindSceneTexture()
  {
    auto* scene_texture = GetRenderGraph().GetTexture("SceneColor");
    if (scene_texture != nullptr && m_CompositeDescriptorSet != nullptr) {
      m_CompositeDescriptorSet->WriteCombinedImageSampler(
          0, scene_texture, m_CompositeSampler.get());
    }
  }

  // Scene
  std::unique_ptr<AssetManager> m_AssetManager;
  std::unique_ptr<SceneRenderer> m_SceneRenderer;
  std::unique_ptr<Scene> m_Scene;

  // Camera
  Camera m_Camera;
  std::unique_ptr<FPSCameraController> m_FPSController;

  // Composite pass resources
  std::shared_ptr<RHIDescriptorSetLayout> m_CompositeDescriptorSetLayout;
  std::shared_ptr<RHIPipelineLayout> m_CompositePipelineLayout;
  std::unique_ptr<RHIShaderModule> m_CompositeVS;
  std::unique_ptr<RHIShaderModule> m_CompositeFS;
  std::unique_ptr<RHISampler> m_CompositeSampler;
  std::unique_ptr<RHIDescriptorSet> m_CompositeDescriptorSet;

  // Resize tracking
  uint32_t m_LastWidth {0};
  uint32_t m_LastHeight {0};
};

auto CreateApplication() -> std::unique_ptr<Application>
{
  return std::make_unique<RenderGraphDemoApp>();
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
