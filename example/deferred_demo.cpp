#include <memory>

#include <imgui.h>
#include <linalg/vec.hpp>

#include "Core/Application.hpp"
#include "Core/Input.hpp"
#include "Core/Logger.hpp"
#include "Core/Window.hpp"
#include "Renderer/Asset/AssetManager.hpp"
#include "Renderer/Camera.hpp"
#include "Renderer/CameraController.hpp"
#include "Renderer/Model/Model.hpp"
#include "Renderer/RHI/RHIBuffer.hpp"
#include "Renderer/RHI/RHICommandBuffer.hpp"
#include "Renderer/RHI/RHIDescriptorSet.hpp"
#include "Renderer/RHI/RHIDevice.hpp"
#include "Renderer/RHI/RHISampler.hpp"
#include "Renderer/RHI/RHISwapchain.hpp"
#include "Renderer/RenderGraph.hpp"
#include "Renderer/Scene/Scene.hpp"
#include "Renderer/Scene/SceneNode.hpp"
#include "Renderer/Scene/SceneRenderer.hpp"
#include "Renderer/ShaderCompiler.hpp"
#include "Renderer/ShaderReflection.hpp"
#include "UI/RHIImGui.hpp"

struct DebugParamsUBO
{
  int32_t DisplayMode {0};
  float NearPlane {0.01F};
  float FarPlane {1000.0F};
  float Padding {0.0F};
};

class DeferredDemoApp : public Application
{
public:
  DeferredDemoApp() = default;

protected:
  void OnInit() override
  {
    Logger::Info("DeferredDemoApp::OnInit - Setting up deferred rendering demo");

    m_AssetManager = std::make_unique<AssetManager>(GetDevice());
    m_SceneRenderer = std::make_unique<SceneRenderer>(
        GetDevice(), GetRendererConfig().API, "shaders/gbuffer.slang");
    m_AssetManager->SetMaterialDescriptorSetLayout(
        m_SceneRenderer->GetSetLayout("material"));

    m_Scene = std::make_unique<Scene>("Deferred Rendering Demo Scene");

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

    setupDebugShader();
    setupRenderGraph();

    Logger::Info("Deferred demo initialized with {} nodes",
                 m_Scene->GetNodeCount());
    Logger::Info(
        "Controls: ESC=Exit, 1-5=GBuffer layers, G=Grid, F1=Settings");
  }

  void OnUpdate(float delta_time) override
  {
    if (Input::IsKeyPressed(KeyCode::Escape)) {
      GetWindow().RequestClose();
      return;
    }

    // Display mode key shortcuts
    if (Input::IsKeyPressed(KeyCode::Num1)) {
      m_DisplayMode = 0;
    }
    if (Input::IsKeyPressed(KeyCode::Num2)) {
      m_DisplayMode = 1;
    }
    if (Input::IsKeyPressed(KeyCode::Num3)) {
      m_DisplayMode = 2;
    }
    if (Input::IsKeyPressed(KeyCode::Num4)) {
      m_DisplayMode = 3;
    }
    if (Input::IsKeyPressed(KeyCode::Num5)) {
      m_DisplayMode = 4;
    }

    // Toggle grid
    if (Input::IsKeyPressed(KeyCode::G)) {
      m_ShowGrid = !m_ShowGrid;
    }

    m_FPSController->Update(delta_time);

    auto* swapchain = GetDevice().GetSwapchain();
    const float aspect = static_cast<float>(swapchain->GetWidth())
        / static_cast<float>(swapchain->GetHeight());
    m_Camera.SetAspectRatio(aspect);

    m_Scene->UpdateTransforms();

    if (Input::IsMouseButtonPressed(MouseButton::Left)) {
      const auto mouse_pos = Input::GetMousePosition();
      const auto ray =
          m_Camera.ScreenPointToRay(mouse_pos.x(),
                                    mouse_pos.y(),
                                    static_cast<float>(swapchain->GetWidth()),
                                    static_cast<float>(swapchain->GetHeight()));
      GetImGui().SetSelectedNode(m_Scene->PickNode(ray));
    }

    updateDebugParams();

    if (swapchain->GetWidth() != m_LastWidth
        || swapchain->GetHeight() != m_LastHeight)
    {
      m_LastWidth = swapchain->GetWidth();
      m_LastHeight = swapchain->GetHeight();
      GetDevice().WaitIdle();
      GetRenderGraph().Resize(GetDevice(), m_LastWidth, m_LastHeight);

      rebindDebugTexture();
    }
  }

  void OnDestroy() override
  {
    Logger::Info("DeferredDemoApp::OnDestroy - Cleaning up");
    m_FPSController.reset();
    m_DebugParamsDescriptorSet.reset();
    m_DebugTextureDescriptorSet.reset();
    m_DebugPipelineLayout.reset();
    m_DebugReflectedLayout.SetLayouts.clear();
    m_DebugReflectedLayout.ParameterSetIndex.clear();
    m_DebugVS.reset();
    m_DebugFS.reset();
    m_DebugSampler.reset();
    m_DebugParamsBuffer.reset();
    m_SceneRenderer.reset();
    m_Scene.reset();
    m_AssetManager.reset();
  }

private:
  void setupDebugShader()
  {
    // Compile the debug visualization shader
    const auto shader_result = ShaderCompiler::Compile(
        "shaders/gbuffer_debug.slang", GetRendererConfig().API);

    // Reflect the layout to get set indices for "params" and "debugTexture"
    m_DebugReflectedLayout = CreatePipelineLayoutFromReflection(
        GetDevice(), shader_result.Reflection);

    m_DebugParamsSetIndex =
        m_DebugReflectedLayout.GetSetIndex("params");
    m_DebugTextureSetIndex =
        m_DebugReflectedLayout.GetSetIndex("debugTexture");

    // Create pipeline layout from reflected set layouts
    m_DebugPipelineLayout =
        GetDevice().CreatePipelineLayout(m_DebugReflectedLayout.SetLayouts);

    // Create shader modules
    ShaderModuleDesc vertex_desc {};
    vertex_desc.Stage = ShaderStage::Vertex;
    vertex_desc.SPIRVCode = shader_result.GetSPIRV(ShaderType::Vertex);
    vertex_desc.GLSLCode = shader_result.GetGLSL(ShaderType::Vertex);
    vertex_desc.EntryPoint = "vertexMain";
    vertex_desc.SetLayouts = m_DebugReflectedLayout.SetLayouts;
    m_DebugVS = GetDevice().CreateShaderModule(vertex_desc);

    ShaderModuleDesc fragment_desc {};
    fragment_desc.Stage = ShaderStage::Fragment;
    fragment_desc.SPIRVCode = shader_result.GetSPIRV(ShaderType::Fragment);
    fragment_desc.GLSLCode = shader_result.GetGLSL(ShaderType::Fragment);
    fragment_desc.EntryPoint = "fragmentMain";
    fragment_desc.SetLayouts = m_DebugReflectedLayout.SetLayouts;
    m_DebugFS = GetDevice().CreateShaderModule(fragment_desc);

    // Create sampler
    SamplerDesc sampler_desc {};
    sampler_desc.MinFilter = Filter::Linear;
    sampler_desc.MagFilter = Filter::Linear;
    sampler_desc.MaxLod = 0.0F;
    m_DebugSampler = GetDevice().CreateSampler(sampler_desc);

    // Create UBO buffer for DebugParams (16 bytes)
    BufferDesc buffer_desc {};
    buffer_desc.Size = sizeof(DebugParamsUBO);
    buffer_desc.Usage = BufferUsage::Uniform;
    buffer_desc.CPUVisible = true;
    m_DebugParamsBuffer = GetDevice().CreateBuffer(buffer_desc);

    // Create descriptor sets
    m_DebugParamsDescriptorSet = GetDevice().CreateDescriptorSet(
        m_DebugReflectedLayout.GetSetLayout("params"));
    m_DebugTextureDescriptorSet = GetDevice().CreateDescriptorSet(
        m_DebugReflectedLayout.GetSetLayout("debugTexture"));

    // Write the UBO to the params descriptor set
    m_DebugParamsDescriptorSet->WriteBuffer(
        0, m_DebugParamsBuffer.get(), 0, sizeof(DebugParamsUBO));
  }

  void setupRenderGraph()
  {
    auto* swapchain = GetDevice().GetSwapchain();
    m_LastWidth = swapchain->GetWidth();
    m_LastHeight = swapchain->GetHeight();

    auto& graph = GetRenderGraph();

    graph.AddResource({
        .Name = "GBuffer.Albedo",
        .Width = m_LastWidth,
        .Height = m_LastHeight,
        .ColorFormat = TextureFormat::RGBA8Srgb,
        .HasDepth = false,
        .IsDepth = false,
    });
    graph.AddResource({
        .Name = "GBuffer.Normals",
        .Width = m_LastWidth,
        .Height = m_LastHeight,
        .ColorFormat = TextureFormat::RGBA16F,
        .HasDepth = false,
        .IsDepth = false,
    });
    graph.AddResource({
        .Name = "GBuffer.Depth",
        .Width = m_LastWidth,
        .Height = m_LastHeight,
        .DepthFormat = TextureFormat::Depth32F,
        .HasDepth = false,
        .IsDepth = true,
    });

    // GeometryPass - writes all 3 GBuffer resources (2 color + depth)
    graph.AddPass({
        .Name = "GeometryPass",
        .Outputs = {"GBuffer.Albedo", "GBuffer.Normals", "GBuffer.Depth"},
        .ColorAttachments =
            {AttachmentInfo {.ColorLoadOp = LoadOp::Clear,
                             .ClearColor = {.R = 0, .G = 0, .B = 0, .A = 0}},
             AttachmentInfo {
                 .ColorLoadOp = LoadOp::Clear,
                 .ClearColor = {.R = 0, .G = 0, .B = 0, .A = 0.5F}}},
        .ColorAttachmentCount = 2,
        .UseDepth = true,
        .DepthStencil =
            {.DepthLoadOp = LoadOp::Clear,
             .DepthStoreOp = StoreOp::Store,
             .ClearDepthStencil = {.Depth = 1.0F}},
        .Execute =
            [this](RHICommandBuffer& cmd)
        {
          m_SceneRenderer->SetWireframe(GetImGui().IsWireframe());
          m_SceneRenderer->BeginFrame(m_Camera);
          m_SceneRenderer->RenderScene(cmd, *m_Scene);
        },
    });

    // CompositePass - reads selected GBuffer layer, writes to backbuffer
    graph.AddPass({
        .Name = "CompositePass",
        .Inputs = {"GBuffer.Albedo", "GBuffer.Normals", "GBuffer.Depth"},
        .Outputs = {RenderGraph::BACKBUFFER},
        .ColorAttachments =
            {AttachmentInfo {.ColorLoadOp = LoadOp::Clear,
                             .ClearColor = {.R = 0, .G = 0, .B = 0, .A = 1}}},
        .ColorAttachmentCount = 1,
        .UseDepth = false,
        .Execute =
            [this](RHICommandBuffer& cmd)
        {
          cmd.SetPrimitiveTopology(PrimitiveTopology::TriangleList);
          cmd.BindShaders(m_DebugVS.get(), m_DebugFS.get());
          cmd.BindDescriptorSet(m_DebugParamsSetIndex,
                                *m_DebugParamsDescriptorSet,
                                *m_DebugPipelineLayout);
          cmd.BindDescriptorSet(m_DebugTextureSetIndex,
                                *m_DebugTextureDescriptorSet,
                                *m_DebugPipelineLayout);
          cmd.Draw(3, 1, 0, 0);

          renderDebugUI();
          GetImGui().RenderPanels(*m_Scene);
          GetImGui().EndFrame();
        },
    });

    graph.Compile(GetDevice());
    rebindDebugTexture();
  }

  [[nodiscard]] static auto getTextureNameForMode(int mode) -> const char*
  {
    switch (mode) {
      case 0:  // Albedo
      case 3:  // Metallic (stored in albedo alpha)
        return "GBuffer.Albedo";
      case 1:  // Normals
      case 4:  // Roughness (stored in normals alpha)
        return "GBuffer.Normals";
      case 2:  // Depth
        return "GBuffer.Depth";
      default:
        return "GBuffer.Albedo";
    }
  }

  void rebindDebugTexture()
  {
    const char* tex_name = getTextureNameForMode(m_DisplayMode);
    auto* texture = GetRenderGraph().GetTexture(tex_name);
    if (texture != nullptr && m_DebugTextureDescriptorSet != nullptr) {
      m_DebugTextureDescriptorSet->WriteCombinedImageSampler(
          0, texture, m_DebugSampler.get());
    }

    // Update grid textures for ImGui thumbnails
    m_GridTextures[0] = GetRenderGraph().GetTexture("GBuffer.Albedo");
    m_GridTextures[1] = GetRenderGraph().GetTexture("GBuffer.Normals");
    m_GridTextures[2] = GetRenderGraph().GetTexture("GBuffer.Depth");

    // Register textures for ImGui rendering
    for (int i = 0; i < 3; ++i) {
      if (m_GridTextures[i] != nullptr) {
        m_GridImGuiTextures[i] = reinterpret_cast<ImTextureID>(
            GetImGui().RegisterTexture(m_GridTextures[i]));
      }
    }
  }

  void updateDebugParams()
  {
    DebugParamsUBO params {};
    params.DisplayMode = m_DisplayMode;
    params.NearPlane = 0.01F;
    params.FarPlane = 1000.0F;
    params.Padding = 0.0F;

    m_DebugParamsBuffer->Upload(&params, sizeof(DebugParamsUBO), 0);

    // Rebind texture if display mode changed (different GBuffer layer)
    static int last_mode = -1;
    if (last_mode != m_DisplayMode) {
      last_mode = m_DisplayMode;
      GetDevice().WaitIdle();
      rebindDebugTexture();
    }
  }

  void renderDebugUI()
  {
    static const char* names[] = {
        "Albedo", "Normals", "Depth", "Metallic", "Roughness"};

    ImGui::Begin("GBuffer Debug");
    if (ImGui::BeginCombo("Display", names[m_DisplayMode])) {
      for (int i = 0; i < 5; ++i) {
        if (ImGui::Selectable(names[i], m_DisplayMode == i)) {
          m_DisplayMode = i;
        }
        if (m_DisplayMode == i) {
          ImGui::SetItemDefaultFocus();
        }
      }
      ImGui::EndCombo();
    }
    ImGui::Text("Keys: 1-5 layers, G grid");
    ImGui::End();

    if (m_ShowGrid) {
      ImGui::Begin("GBuffer Grid", &m_ShowGrid);
      float thumb = 140.0F;
      const char* labels[] = {"Albedo", "Normals", "Depth"};
      for (int i = 0; i < 3; ++i) {
        if (m_GridImGuiTextures[i] != 0) {
          ImGui::BeginGroup();
          ImGui::Text("%s", labels[i]);
          if (ImGui::ImageButton(
                  labels[i], m_GridImGuiTextures[i], ImVec2(thumb, thumb)))
          {
            m_DisplayMode = i;
          }
          ImGui::EndGroup();
          if (i < 2) {
            ImGui::SameLine();
          }
        }
      }
      ImGui::End();
    }
  }

  // Scene
  std::unique_ptr<AssetManager> m_AssetManager;
  std::unique_ptr<SceneRenderer> m_SceneRenderer;
  std::unique_ptr<Scene> m_Scene;

  // Camera
  Camera m_Camera;
  std::unique_ptr<FPSCameraController> m_FPSController;

  // Debug shader resources
  ReflectedPipelineLayout m_DebugReflectedLayout;
  std::shared_ptr<RHIPipelineLayout> m_DebugPipelineLayout;
  std::unique_ptr<RHIShaderModule> m_DebugVS;
  std::unique_ptr<RHIShaderModule> m_DebugFS;
  std::unique_ptr<RHISampler> m_DebugSampler;
  std::unique_ptr<RHIBuffer> m_DebugParamsBuffer;
  std::unique_ptr<RHIDescriptorSet> m_DebugParamsDescriptorSet;
  std::unique_ptr<RHIDescriptorSet> m_DebugTextureDescriptorSet;
  uint32_t m_DebugParamsSetIndex {0};
  uint32_t m_DebugTextureSetIndex {0};

  // Display state
  int m_DisplayMode {0};
  bool m_ShowGrid {false};
  RHITexture* m_GridTextures[3] {nullptr, nullptr, nullptr};
  ImTextureID m_GridImGuiTextures[3] {0, 0, 0};

  // Resize tracking
  uint32_t m_LastWidth {0};
  uint32_t m_LastHeight {0};
};

auto CreateApplication() -> std::unique_ptr<Application>
{
  return std::make_unique<DeferredDemoApp>();
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
