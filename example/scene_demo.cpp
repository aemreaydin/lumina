#include <memory>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include "Core/Application.hpp"
#include "Core/Input.hpp"
#include "Core/Logger.hpp"
#include "Core/Window.hpp"
#include "Renderer/Asset/AssetManager.hpp"
#include "Renderer/Camera.hpp"
#include "Renderer/CameraController.hpp"
#include "Renderer/Model/Model.hpp"
#include "Renderer/RHI/RHIDevice.hpp"
#include "Renderer/RHI/RHISwapchain.hpp"
#include "Renderer/Scene/Scene.hpp"
#include "Renderer/Scene/SceneNode.hpp"
#include "Renderer/Scene/SceneRenderer.hpp"
#include "UI/RHIImGui.hpp"

class SceneDemoApp : public Application
{
public:
  SceneDemoApp() = default;

protected:
  void OnInit() override
  {
    Logger::Info("SceneDemoApp::OnInit - Setting up scene");

    m_AssetManager = std::make_unique<AssetManager>(GetDevice());

    m_SceneRenderer = std::make_unique<SceneRenderer>(GetDevice());

    // Pass the reflected material layout from shader to AssetManager
    m_AssetManager->SetMaterialDescriptorSetLayout(
        m_SceneRenderer->GetSetLayout("material"));

    m_Scene = std::make_unique<Scene>("Demo Scene");

    auto model = m_AssetManager->LoadModel("volleyball/volleyball.obj");
    if (!model) {
      Logger::Error("Failed to load model!");
      return;
    }

    auto* node1 = m_Scene->CreateNode("Volleyball1");
    node1->SetModel(model);
    node1->SetPosition(glm::vec3(0.0F, 0.0F, 0.0F));
    node1->SetScale(0.1F);

    auto* node2 = m_Scene->CreateNode("Volleyball2");
    node2->SetModel(model);
    node2->SetPosition(glm::vec3(5.0F, 0.0F, 0.0F));
    node2->SetScale(0.1F);

    auto* node3 = m_Scene->CreateNode("Volleyball3");
    node3->SetModel(model);
    node3->SetPosition(glm::vec3(-5.0F, 0.0F, 0.0F));
    node3->SetScale(0.1F);

    auto* child = m_Scene->CreateNode("ChildBall", node1);
    child->SetModel(model);
    child->SetPosition(glm::vec3(0.0F, 30.0F, 0.0F));
    child->SetScale(1.0F);

    auto* child1 = m_Scene->CreateNode("ChildBall", node1);
    child1->SetModel(model);
    child1->SetPosition(glm::vec3(0.0F, -30.0F, 0.0F));
    child1->SetScale(1.0F);

    m_Camera.SetPerspective(45.0F, 16.0F / 9.0F, 0.01F, 1000.0F);
    m_Camera.SetPosition(glm::vec3(15.0F, 10.0F, 15.0F));
    m_Camera.SetTarget(glm::vec3(0.0F, 0.0F, 0.0F));

    m_OrbitController = std::make_unique<OrbitCameraController>(&m_Camera);
    m_OrbitController->SetTarget(glm::vec3(0.0F, 0.0F, 0.0F));
    m_OrbitController->SetDistance(20.0F);
    m_OrbitController->SetDistanceLimits(5.0F, 50.0F);

    m_FPSController = std::make_unique<FPSCameraController>(&m_Camera);
    m_FPSController->SetMoveSpeed(10.0F);

    m_ActiveController = m_OrbitController.get();

    // Setup ImGui with camera
    GetImGui().SetCamera(m_Camera);

    Logger::Info("Scene created with {} nodes", m_Scene->GetNodeCount());
    Logger::Info("Controls: 1=Orbit camera, 2=FPS camera, ESC=Exit");
    Logger::Info("          R=Rotate node1");
    Logger::Info("          F1=Settings, F2=Scene Hierarchy");
  }

  void OnUpdate(float delta_time) override
  {
    if (Input::IsKeyPressed(KeyCode::Escape)) {
      GetWindow().RequestClose();
      return;
    }

    if (Input::IsKeyPressed(KeyCode::Num1)) {
      m_ActiveController = m_OrbitController.get();
      Input::SetMouseCaptured(/*captured=*/false);
      Logger::Info("Switched to Orbit camera");
    }

    if (Input::IsKeyPressed(KeyCode::Num2)) {
      m_ActiveController = m_FPSController.get();
      Logger::Info("Switched to FPS camera");
    }

    if (Input::IsKeyDown(KeyCode::R)) {
      auto* node = m_Scene->FindNode("Volleyball1");
      if (node != nullptr) {
        node->GetTransform().RotateEuler(
            glm::vec3(0.0F, 45.0F * delta_time, 0.0F));
      }
    }


    // Viewport picking on left click
    if (Input::IsMouseButtonPressed(MouseButton::Left)) {
      auto* swapchain = GetDevice().GetSwapchain();
      const auto mouse_pos = Input::GetMousePosition();
      const auto ray =
          m_Camera.ScreenPointToRay(mouse_pos.x,
                                    mouse_pos.y,
                                    static_cast<float>(swapchain->GetWidth()),
                                    static_cast<float>(swapchain->GetHeight()));
      GetImGui().SetSelectedNode(m_Scene->PickNode(ray));
    }

    m_ActiveController->Update(delta_time);

    auto* swapchain = GetDevice().GetSwapchain();
    const float aspect = static_cast<float>(swapchain->GetWidth())
        / static_cast<float>(swapchain->GetHeight());
    m_Camera.SetAspectRatio(aspect);

    m_Scene->UpdateTransforms();
  }

  void OnRender(float /*delta_time*/) override
  {
    auto& device = GetDevice();
    auto* cmd = device.GetCurrentCommandBuffer();

    m_SceneRenderer->BeginFrame(m_Camera);
    m_SceneRenderer->RenderScene(*cmd, *m_Scene);

    // Render ImGui panels
    GetImGui().RenderPanels(*m_Scene);
  }

  void OnDestroy() override
  {
    Logger::Info("SceneDemoApp::OnDestroy - Cleaning up");
    m_ActiveController = nullptr;
    m_OrbitController.reset();
    m_FPSController.reset();
    m_SceneRenderer.reset();
    m_Scene.reset();
    m_AssetManager.reset();
  }

private:
  std::unique_ptr<AssetManager> m_AssetManager;
  std::unique_ptr<SceneRenderer> m_SceneRenderer;
  std::unique_ptr<Scene> m_Scene;

  Camera m_Camera;
  std::unique_ptr<OrbitCameraController> m_OrbitController;
  std::unique_ptr<FPSCameraController> m_FPSController;
  CameraController* m_ActiveController {nullptr};
};

auto CreateApplication() -> std::unique_ptr<Application>
{
  return std::make_unique<SceneDemoApp>();
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
