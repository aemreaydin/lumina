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
#include "Renderer/Scene/LightData.hpp"
#include "Renderer/Scene/Scene.hpp"
#include "Renderer/Scene/SceneNode.hpp"
#include "Renderer/Scene/SceneRenderer.hpp"
#include "Renderer/ShaderCompiler.hpp"
#include "Renderer/ShaderReflection.hpp"
#include "UI/RHIImGui.hpp"

// GPU-side structs matching deferred_lighting.slang layout
struct PointLightGPU
{
  linalg::Vec3 Position;
  float Radius;
  linalg::Vec3 Color;
  float Intensity;
};

struct DirectionalLightGPU
{
  linalg::Vec3 Direction;
  float Intensity;
  linalg::Vec3 Color;
  float _pad {0.0F};
};

static constexpr int kMaxPointLights = 64;

struct LightingUBO
{
  PointLightGPU PointLights[kMaxPointLights] {};
  DirectionalLightGPU DirLight {};
  int32_t NumPointLights {0};
  float _pad[3] {0.0F, 0.0F, 0.0F};
};

struct CompositeParamsUBO
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
    Logger::Info("DeferredDemoApp::OnInit - Setting up deferred lighting demo");

    m_AssetManager = std::make_unique<AssetManager>(GetDevice());
    m_SceneRenderer = std::make_unique<SceneRenderer>(
        GetDevice(), GetRendererConfig().API, "shaders/gbuffer.slang");
    m_AssetManager->SetMaterialDescriptorSetLayout(
        m_SceneRenderer->GetSetLayout("material"));

    m_Scene = std::make_unique<Scene>("Deferred Lighting Demo Scene");

    auto lion_head = m_AssetManager->LoadModel("lion_head/lion_head_4k.obj");
    if (!lion_head) {
      throw std::runtime_error("Failed to load lion_head model");
    }
    auto coffee_table =
        m_AssetManager->LoadModel("coffee_table/gothic_coffee_table_4k.obj");
    if (!coffee_table) {
      throw std::runtime_error("Failed to load coffee_table model");
    }
    auto chair =
        m_AssetManager->LoadModel("chair/mid_century_lounge_chair_4k.obj");
    if (!chair) {
      throw std::runtime_error("Failed to load chair model");
    }

    auto* node1 = m_Scene->CreateNode("Lion Head");
    node1->SetModel(lion_head);
    node1->SetPosition(linalg::Vec3 {0.0F, 0.0F, 0.0F});
    node1->SetScale(10.0F);

    auto* node2 = m_Scene->CreateNode("Coffee Table");
    node2->SetModel(coffee_table);
    node2->SetPosition(linalg::Vec3 {0.0F, 0.0F, -3.0F});
    node2->SetScale(6.0F);

    auto* node3 = m_Scene->CreateNode("Chair");
    node3->SetModel(chair);
    node3->SetPosition(linalg::Vec3 {-5.0F, 0.0F, -3.0F});
    node3->SetRotationEuler(linalg::Vec3 {0.0F, 0.0F, -90.0F});
    node3->SetScale(3.0F);

    auto* node4 = m_Scene->CreateNode("Chair");
    node3->SetModel(chair);
    node3->SetPosition(linalg::Vec3 {5.0F, 0.0F, -3.0F});
    node3->SetRotationEuler(linalg::Vec3 {0.0F, 0.0F, 90.0F});
    node3->SetScale(3.0F);

    auto* node5 = m_Scene->CreateNode("Chair");
    node3->SetModel(chair);
    node3->SetPosition(linalg::Vec3 {0.0F, 5.0F, -3.0F});
    node3->SetRotationEuler(linalg::Vec3 {0.0F, 0.0F, 0.0F});
    node3->SetScale(3.0F);

    auto* node6 = m_Scene->CreateNode("Chair");
    node3->SetModel(chair);
    node3->SetPosition(linalg::Vec3 {0.0F, -5.0F, -3.0F});
    node3->SetRotationEuler(linalg::Vec3 {0.0F, 0.0F, 180.0F});
    node3->SetScale(3.0F);

    setupLights();

    m_Camera.SetPerspective(45.0F, 16.0F / 9.0F, 0.01F, 1000.0F);
    m_Camera.SetPosition(linalg::Vec3 {0.0F, 15.0F, 5.0F});
    m_Camera.SetTarget(linalg::Vec3 {0.0F, 0.0F, 0.0F});

    m_FPSController = std::make_unique<FPSCameraController>(&m_Camera);

    GetImGui().SetCamera(m_Camera);

    setupLightingShader();
    setupCompositeShader();
    setupRenderGraph();

    Logger::Info("Deferred lighting demo initialized with {} nodes",
                 m_Scene->GetNodeCount());
    Logger::Info(
        "Controls: ESC=Exit, 1-7=Display modes, G=Grid, F1=Settings");
  }

  void OnUpdate(float delta_time) override
  {
    if (Input::IsKeyPressed(KeyCode::Escape)) {
      GetWindow().RequestClose();
      return;
    }

    // Display mode key shortcuts (7 modes)
    for (int i = 0; i < 7; ++i) {
      if (Input::IsKeyPressed(
              static_cast<KeyCode>(static_cast<int>(KeyCode::Num1) + i)))
      {
        m_DisplayMode = i;
      }
    }

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

    updateLightingUBO();
    updateCompositeParams();

    if (swapchain->GetWidth() != m_LastWidth
        || swapchain->GetHeight() != m_LastHeight)
    {
      m_LastWidth = swapchain->GetWidth();
      m_LastHeight = swapchain->GetHeight();
      GetDevice().WaitIdle();
      GetRenderGraph().Resize(GetDevice(), m_LastWidth, m_LastHeight);

      rebindTextures();
    }
  }

  void OnDestroy() override
  {
    Logger::Info("DeferredDemoApp::OnDestroy - Cleaning up");
    m_FPSController.reset();

    // Lighting shader resources
    m_LightGBufferDescriptorSet.reset();
    m_LightingDataDescriptorSet.reset();
    m_LightCameraDescriptorSet.reset();
    m_LightPipelineLayout.reset();
    m_LightReflectedLayout.SetLayouts.clear();
    m_LightReflectedLayout.ParameterSetIndex.clear();
    m_LightVS.reset();
    m_LightFS.reset();
    m_LightingUBOBuffer.reset();
    m_LightCameraUBOBuffer.reset();

    // Composite shader resources
    m_CompositeParamsDescriptorSet.reset();
    m_CompositeTextureDescriptorSet.reset();
    m_CompositePipelineLayout.reset();
    m_CompositeReflectedLayout.SetLayouts.clear();
    m_CompositeReflectedLayout.ParameterSetIndex.clear();
    m_CompositeVS.reset();
    m_CompositeFS.reset();
    m_CompositeParamsBuffer.reset();

    m_Sampler.reset();
    m_SceneRenderer.reset();
    m_Scene.reset();
    m_AssetManager.reset();
  }

private:
  void setupLights()
  {
    // Directional light (sun)
    m_SunNode = m_Scene->CreateNode("Sun");
    LightComponent sun_light;
    sun_light.LightType = LightComponent::Type::Directional;
    sun_light.Direction = linalg::Vec3 {-0.5F, -1.0F, -0.3F};
    sun_light.Color = linalg::Vec3 {1.0F, 0.95F, 0.9F};
    sun_light.Intensity = 2.0F;
    m_SunNode->SetLight(sun_light);

    // Red point light
    auto* red_light = m_Scene->CreateNode("Red Light");
    red_light->SetPosition(linalg::Vec3 {-3.0F, 5.0F, 2.0F});
    LightComponent red;
    red.LightType = LightComponent::Type::Point;
    red.Color = linalg::Vec3 {1.0F, 0.2F, 0.1F};
    red.Intensity = 3.0F;
    red.Radius = 15.0F;
    red_light->SetLight(red);
    m_PointLightNodes.push_back(red_light);

    // Blue point light
    auto* blue_light = m_Scene->CreateNode("Blue Light");
    blue_light->SetPosition(linalg::Vec3 {3.0F, 5.0F, -2.0F});
    LightComponent blue;
    blue.LightType = LightComponent::Type::Point;
    blue.Color = linalg::Vec3 {0.1F, 0.3F, 1.0F};
    blue.Intensity = 3.0F;
    blue.Radius = 15.0F;
    blue_light->SetLight(blue);
    m_PointLightNodes.push_back(blue_light);

    // White point light
    auto* white_light = m_Scene->CreateNode("White Light");
    white_light->SetPosition(linalg::Vec3 {0.0F, 8.0F, 0.0F});
    LightComponent white;
    white.LightType = LightComponent::Type::Point;
    white.Color = linalg::Vec3 {1.0F, 1.0F, 1.0F};
    white.Intensity = 2.0F;
    white.Radius = 20.0F;
    white_light->SetLight(white);
    m_PointLightNodes.push_back(white_light);
  }

  void setupLightingShader()
  {
    const auto shader_result = ShaderCompiler::Compile(
        "shaders/deferred_lighting.slang", GetRendererConfig().API);

    m_LightReflectedLayout = CreatePipelineLayoutFromReflection(
        GetDevice(), shader_result.Reflection);

    m_LightPipelineLayout =
        GetDevice().CreatePipelineLayout(m_LightReflectedLayout.SetLayouts);

    ShaderModuleDesc vertex_desc {};
    vertex_desc.Stage = ShaderStage::Vertex;
    vertex_desc.SPIRVCode = shader_result.GetSPIRV(ShaderType::Vertex);
    vertex_desc.GLSLCode = shader_result.GetGLSL(ShaderType::Vertex);
    vertex_desc.EntryPoint = "vertexMain";
    vertex_desc.SetLayouts = m_LightReflectedLayout.SetLayouts;
    m_LightVS = GetDevice().CreateShaderModule(vertex_desc);

    ShaderModuleDesc fragment_desc {};
    fragment_desc.Stage = ShaderStage::Fragment;
    fragment_desc.SPIRVCode = shader_result.GetSPIRV(ShaderType::Fragment);
    fragment_desc.GLSLCode = shader_result.GetGLSL(ShaderType::Fragment);
    fragment_desc.EntryPoint = "fragmentMain";
    fragment_desc.SetLayouts = m_LightReflectedLayout.SetLayouts;
    m_LightFS = GetDevice().CreateShaderModule(fragment_desc);

    // Create sampler (shared between passes)
    SamplerDesc sampler_desc {};
    sampler_desc.MinFilter = Filter::Linear;
    sampler_desc.MagFilter = Filter::Linear;
    sampler_desc.MaxLod = 0.0F;
    m_Sampler = GetDevice().CreateSampler(sampler_desc);

    // Lighting UBO buffer
    BufferDesc lighting_buf_desc {};
    lighting_buf_desc.Size = sizeof(LightingUBO);
    lighting_buf_desc.Usage = BufferUsage::Uniform;
    lighting_buf_desc.CPUVisible = true;
    m_LightingUBOBuffer = GetDevice().CreateBuffer(lighting_buf_desc);

    // Camera UBO buffer for lighting pass
    BufferDesc camera_buf_desc {};
    camera_buf_desc.Size = sizeof(CameraUBO);
    camera_buf_desc.Usage = BufferUsage::Uniform;
    camera_buf_desc.CPUVisible = true;
    m_LightCameraUBOBuffer = GetDevice().CreateBuffer(camera_buf_desc);

    // Descriptor sets
    m_LightGBufferDescriptorSet = GetDevice().CreateDescriptorSet(
        m_LightReflectedLayout.GetSetLayout("gbuffer"));

    m_LightingDataDescriptorSet = GetDevice().CreateDescriptorSet(
        m_LightReflectedLayout.GetSetLayout("lighting"));
    m_LightingDataDescriptorSet->WriteBuffer(
        0, m_LightingUBOBuffer.get(), 0, sizeof(LightingUBO));

    m_LightCameraDescriptorSet = GetDevice().CreateDescriptorSet(
        m_LightReflectedLayout.GetSetLayout("camera"));
    m_LightCameraDescriptorSet->WriteBuffer(
        0, m_LightCameraUBOBuffer.get(), 0, sizeof(CameraUBO));
  }

  void setupCompositeShader()
  {
    const auto shader_result = ShaderCompiler::Compile(
        "shaders/deferred_composite.slang", GetRendererConfig().API);

    m_CompositeReflectedLayout = CreatePipelineLayoutFromReflection(
        GetDevice(), shader_result.Reflection);

    m_CompositePipelineLayout =
        GetDevice().CreatePipelineLayout(m_CompositeReflectedLayout.SetLayouts);

    ShaderModuleDesc vertex_desc {};
    vertex_desc.Stage = ShaderStage::Vertex;
    vertex_desc.SPIRVCode = shader_result.GetSPIRV(ShaderType::Vertex);
    vertex_desc.GLSLCode = shader_result.GetGLSL(ShaderType::Vertex);
    vertex_desc.EntryPoint = "vertexMain";
    vertex_desc.SetLayouts = m_CompositeReflectedLayout.SetLayouts;
    m_CompositeVS = GetDevice().CreateShaderModule(vertex_desc);

    ShaderModuleDesc fragment_desc {};
    fragment_desc.Stage = ShaderStage::Fragment;
    fragment_desc.SPIRVCode = shader_result.GetSPIRV(ShaderType::Fragment);
    fragment_desc.GLSLCode = shader_result.GetGLSL(ShaderType::Fragment);
    fragment_desc.EntryPoint = "fragmentMain";
    fragment_desc.SetLayouts = m_CompositeReflectedLayout.SetLayouts;
    m_CompositeFS = GetDevice().CreateShaderModule(fragment_desc);

    // Composite params UBO buffer
    BufferDesc params_buf_desc {};
    params_buf_desc.Size = sizeof(CompositeParamsUBO);
    params_buf_desc.Usage = BufferUsage::Uniform;
    params_buf_desc.CPUVisible = true;
    m_CompositeParamsBuffer = GetDevice().CreateBuffer(params_buf_desc);

    // Descriptor sets
    m_CompositeParamsDescriptorSet = GetDevice().CreateDescriptorSet(
        m_CompositeReflectedLayout.GetSetLayout("params"));
    m_CompositeParamsDescriptorSet->WriteBuffer(
        0, m_CompositeParamsBuffer.get(), 0, sizeof(CompositeParamsUBO));

    m_CompositeTextureDescriptorSet = GetDevice().CreateDescriptorSet(
        m_CompositeReflectedLayout.GetSetLayout("textures"));
  }

  void setupRenderGraph()
  {
    auto* swapchain = GetDevice().GetSwapchain();
    m_LastWidth = swapchain->GetWidth();
    m_LastHeight = swapchain->GetHeight();

    auto& graph = GetRenderGraph();

    // GBuffer resources (same as Phase 1)
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

    // HDR lighting output
    graph.AddResource({
        .Name = "LitScene",
        .Width = m_LastWidth,
        .Height = m_LastHeight,
        .ColorFormat = TextureFormat::RGBA16F,
        .HasDepth = false,
        .IsDepth = false,
    });

    // GeometryPass - writes GBuffer
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

    // LightingPass - reads GBuffer, writes LitScene
    graph.AddPass({
        .Name = "LightingPass",
        .Inputs = {"GBuffer.Albedo", "GBuffer.Normals", "GBuffer.Depth"},
        .Outputs = {"LitScene"},
        .ColorAttachments =
            {AttachmentInfo {.ColorLoadOp = LoadOp::Clear,
                             .ClearColor = {.R = 0, .G = 0, .B = 0, .A = 1}}},
        .ColorAttachmentCount = 1,
        .UseDepth = false,
        .Execute =
            [this](RHICommandBuffer& cmd)
        {
          cmd.SetPrimitiveTopology(PrimitiveTopology::TriangleList);
          cmd.BindShaders(m_LightVS.get(), m_LightFS.get());
          cmd.BindDescriptorSet(
              m_LightReflectedLayout.GetSetIndex("gbuffer"),
              *m_LightGBufferDescriptorSet, *m_LightPipelineLayout);
          cmd.BindDescriptorSet(
              m_LightReflectedLayout.GetSetIndex("lighting"),
              *m_LightingDataDescriptorSet, *m_LightPipelineLayout);
          cmd.BindDescriptorSet(
              m_LightReflectedLayout.GetSetIndex("camera"),
              *m_LightCameraDescriptorSet, *m_LightPipelineLayout);
          cmd.Draw(3, 1, 0, 0);
        },
    });

    // CompositePass - reads LitScene + GBuffer, writes backbuffer
    graph.AddPass({
        .Name = "CompositePass",
        .Inputs = {"LitScene", "GBuffer.Albedo", "GBuffer.Normals",
                   "GBuffer.Depth"},
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
          cmd.BindShaders(m_CompositeVS.get(), m_CompositeFS.get());
          cmd.BindDescriptorSet(
              m_CompositeReflectedLayout.GetSetIndex("params"),
              *m_CompositeParamsDescriptorSet, *m_CompositePipelineLayout);
          cmd.BindDescriptorSet(
              m_CompositeReflectedLayout.GetSetIndex("textures"),
              *m_CompositeTextureDescriptorSet, *m_CompositePipelineLayout);
          cmd.Draw(3, 1, 0, 0);

          renderDebugUI();
          GetImGui().RenderPanels(*m_Scene);
          GetImGui().EndFrame();
        },
    });

    graph.Compile(GetDevice());
    rebindTextures();
  }

  void rebindTextures()
  {
    auto& graph = GetRenderGraph();

    auto* albedo_tex = graph.GetTexture("GBuffer.Albedo");
    auto* normals_tex = graph.GetTexture("GBuffer.Normals");
    auto* depth_tex = graph.GetTexture("GBuffer.Depth");
    auto* lit_scene_tex = graph.GetTexture("LitScene");

    // Lighting pass - GBuffer textures
    if (albedo_tex != nullptr && m_LightGBufferDescriptorSet != nullptr) {
      m_LightGBufferDescriptorSet->WriteCombinedImageSampler(
          0, albedo_tex, m_Sampler.get());
      m_LightGBufferDescriptorSet->WriteCombinedImageSampler(
          1, normals_tex, m_Sampler.get());
      m_LightGBufferDescriptorSet->WriteCombinedImageSampler(
          2, depth_tex, m_Sampler.get());
    }

    // Composite pass - all textures
    if (lit_scene_tex != nullptr
        && m_CompositeTextureDescriptorSet != nullptr)
    {
      m_CompositeTextureDescriptorSet->WriteCombinedImageSampler(
          0, lit_scene_tex, m_Sampler.get());
      m_CompositeTextureDescriptorSet->WriteCombinedImageSampler(
          1, albedo_tex, m_Sampler.get());
      m_CompositeTextureDescriptorSet->WriteCombinedImageSampler(
          2, normals_tex, m_Sampler.get());
      m_CompositeTextureDescriptorSet->WriteCombinedImageSampler(
          3, depth_tex, m_Sampler.get());
    }

    // ImGui grid textures
    m_GridTextures[0] = albedo_tex;
    m_GridTextures[1] = normals_tex;
    m_GridTextures[2] = depth_tex;
    m_GridTextures[3] = lit_scene_tex;

    for (int i = 0; i < 4; ++i) {
      if (m_GridTextures[i] != nullptr) {
        m_GridImGuiTextures[i] = reinterpret_cast<ImTextureID>(
            GetImGui().RegisterTexture(m_GridTextures[i]));
      }
    }
  }

  void updateLightingUBO()
  {
    // Upload lighting data
    LightingUBO lighting {};

    auto point_lights = m_Scene->GetPointLights();
    lighting.NumPointLights = static_cast<int32_t>(
        std::min(point_lights.size(), static_cast<size_t>(kMaxPointLights)));

    for (int i = 0; i < lighting.NumPointLights; ++i) {
      lighting.PointLights[i].Position = point_lights[i].Position;
      lighting.PointLights[i].Radius = point_lights[i].Radius;
      lighting.PointLights[i].Color = point_lights[i].Color;
      lighting.PointLights[i].Intensity = point_lights[i].Intensity;
    }

    auto dir_light = m_Scene->GetDirectionalLight();
    if (dir_light) {
      lighting.DirLight.Direction = dir_light->Direction;
      lighting.DirLight.Intensity = dir_light->Intensity;
      lighting.DirLight.Color = dir_light->Color;
    }

    m_LightingUBOBuffer->Upload(&lighting, sizeof(LightingUBO), 0);

    // Upload camera data for lighting pass
    CameraUBO cam {};
    cam.View = m_Camera.GetViewMatrix();
    cam.Projection = m_Camera.GetProjectionMatrix();
    cam.ViewProjection = m_Camera.GetViewProjectionMatrix();
    cam.InverseViewProjection = linalg::inverse(cam.ViewProjection);
    cam.CameraPosition = linalg::Vec4(m_Camera.GetPosition(), 1.0F);

    m_LightCameraUBOBuffer->Upload(&cam, sizeof(CameraUBO), 0);
  }

  void updateCompositeParams()
  {
    CompositeParamsUBO params {};
    params.DisplayMode = m_DisplayMode;
    params.NearPlane = 0.01F;
    params.FarPlane = 1000.0F;
    params.Padding = 0.0F;

    m_CompositeParamsBuffer->Upload(&params, sizeof(CompositeParamsUBO), 0);
  }

  void renderDebugUI()
  {
    static const char* names[] = {"Final (ACES)", "Raw HDR", "Albedo",
                                  "Normals", "Depth", "Metallic", "Roughness"};

    ImGui::Begin("Deferred Lighting");
    if (ImGui::BeginCombo("Display", names[m_DisplayMode])) {
      for (int i = 0; i < 7; ++i) {
        if (ImGui::Selectable(names[i], m_DisplayMode == i)) {
          m_DisplayMode = i;
        }
        if (m_DisplayMode == i) {
          ImGui::SetItemDefaultFocus();
        }
      }
      ImGui::EndCombo();
    }

    ImGui::Text("Keys: 1-7 modes, G grid");
    ImGui::Separator();

    // Directional light controls
    if (m_SunNode != nullptr && m_SunNode->HasLight()) {
      auto lc = *m_SunNode->GetLight();
      if (ImGui::CollapsingHeader("Directional Light",
                                  ImGuiTreeNodeFlags_DefaultOpen))
      {
        float dir[3] = {lc.Direction.x(), lc.Direction.y(), lc.Direction.z()};
        float col[3] = {lc.Color.x(), lc.Color.y(), lc.Color.z()};
        bool changed = false;
        changed |= ImGui::SliderFloat3("Direction", dir, -1.0F, 1.0F);
        changed |= ImGui::ColorEdit3("Color##dir", col);
        changed |= ImGui::SliderFloat("Intensity##dir", &lc.Intensity,
                                      0.0F, 10.0F);
        if (changed) {
          lc.Direction = linalg::Vec3 {dir[0], dir[1], dir[2]};
          lc.Color = linalg::Vec3 {col[0], col[1], col[2]};
          m_SunNode->SetLight(lc);
        }
      }
    }

    // Point light controls
    for (size_t i = 0; i < m_PointLightNodes.size(); ++i) {
      auto* node = m_PointLightNodes[i];
      if (node == nullptr || !node->HasLight()) {
        continue;
      }
      auto lc = *node->GetLight();
      auto pos = node->GetPosition();

      std::string header = node->GetName();
      if (ImGui::CollapsingHeader(header.c_str())) {
        ImGui::PushID(static_cast<int>(i));
        float p[3] = {pos.x(), pos.y(), pos.z()};
        float col[3] = {lc.Color.x(), lc.Color.y(), lc.Color.z()};
        bool changed = false;
        changed |= ImGui::SliderFloat3("Position", p, -20.0F, 20.0F);
        changed |= ImGui::ColorEdit3("Color", col);
        changed |= ImGui::SliderFloat("Intensity", &lc.Intensity,
                                      0.0F, 10.0F);
        changed |= ImGui::SliderFloat("Radius", &lc.Radius, 1.0F, 50.0F);
        if (changed) {
          node->SetPosition(linalg::Vec3 {p[0], p[1], p[2]});
          lc.Color = linalg::Vec3 {col[0], col[1], col[2]};
          node->SetLight(lc);
        }
        ImGui::PopID();
      }
    }

    ImGui::End();

    // Overlay showing current mode in top-left corner
    auto* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(
        ImVec2(viewport->Size.x * 0.5F, 10), ImGuiCond_Always,
        ImVec2(0.5F, 0.0F));
    ImGui::SetNextWindowBgAlpha(0.5F);
    if (ImGui::Begin("##ModeOverlay", nullptr,
                     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove
                         | ImGuiWindowFlags_AlwaysAutoResize
                         | ImGuiWindowFlags_NoFocusOnAppearing
                         | ImGuiWindowFlags_NoNav))
    {
      ImGui::Text("Mode: %s", names[m_DisplayMode]);
    }
    ImGui::End();

    if (m_ShowGrid) {
      ImGui::Begin("Render Targets", &m_ShowGrid);
      float thumb = 140.0F;
      const char* labels[] = {"Albedo", "Normals", "Depth", "LitScene"};
      for (int i = 0; i < 4; ++i) {
        if (m_GridImGuiTextures[i] != 0) {
          ImGui::BeginGroup();
          ImGui::Text("%s", labels[i]);
          if (ImGui::ImageButton(
                  labels[i], m_GridImGuiTextures[i], ImVec2(thumb, thumb)))
          {
            // Map grid index to display mode
            // Grid: 0=Albedo(2), 1=Normals(3), 2=Depth(4), 3=LitScene(0)
            static const int grid_to_mode[] = {2, 3, 4, 0};
            m_DisplayMode = grid_to_mode[i];
          }
          ImGui::EndGroup();
          if (i < 3) {
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

  // Shared sampler
  std::unique_ptr<RHISampler> m_Sampler;

  // Lighting shader resources
  ReflectedPipelineLayout m_LightReflectedLayout;
  std::shared_ptr<RHIPipelineLayout> m_LightPipelineLayout;
  std::unique_ptr<RHIShaderModule> m_LightVS;
  std::unique_ptr<RHIShaderModule> m_LightFS;
  std::unique_ptr<RHIBuffer> m_LightingUBOBuffer;
  std::unique_ptr<RHIBuffer> m_LightCameraUBOBuffer;
  std::unique_ptr<RHIDescriptorSet> m_LightGBufferDescriptorSet;
  std::unique_ptr<RHIDescriptorSet> m_LightingDataDescriptorSet;
  std::unique_ptr<RHIDescriptorSet> m_LightCameraDescriptorSet;

  // Composite shader resources
  ReflectedPipelineLayout m_CompositeReflectedLayout;
  std::shared_ptr<RHIPipelineLayout> m_CompositePipelineLayout;
  std::unique_ptr<RHIShaderModule> m_CompositeVS;
  std::unique_ptr<RHIShaderModule> m_CompositeFS;
  std::unique_ptr<RHIBuffer> m_CompositeParamsBuffer;
  std::unique_ptr<RHIDescriptorSet> m_CompositeParamsDescriptorSet;
  std::unique_ptr<RHIDescriptorSet> m_CompositeTextureDescriptorSet;

  // Light nodes
  SceneNode* m_SunNode {nullptr};
  std::vector<SceneNode*> m_PointLightNodes;

  // Display state
  int m_DisplayMode {0};
  bool m_ShowGrid {false};
  RHITexture* m_GridTextures[4] {nullptr, nullptr, nullptr, nullptr};
  ImTextureID m_GridImGuiTextures[4] {0, 0, 0, 0};

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
