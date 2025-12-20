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

class SceneDemoApp : public Application
{
public:
  SceneDemoApp() = default;

protected:
  void OnInit() override
  {
    Logger::Info("SceneDemoApp::OnInit - Setting up scene");

    // Create asset manager
    m_AssetManager = std::make_unique<AssetManager>(GetDevice());

    // Create scene renderer
    m_SceneRenderer =
        std::make_unique<SceneRenderer>(GetDevice(), *m_AssetManager);

    // Create scene
    m_Scene = std::make_unique<Scene>("Demo Scene");

    // Load model
    auto model = m_AssetManager->LoadModel("volleyball/volleyball.obj");
    if (!model) {
      Logger::Error("Failed to load model!");
      return;
    }

    // Create nodes with the model at different positions
    auto* node1 = m_Scene->CreateNode("Volleyball1");
    node1->SetModel(model);
    node1->SetPosition(glm::vec3(0.0F, 0.0F, 0.0F));

    auto* node2 = m_Scene->CreateNode("Volleyball2");
    node2->SetModel(model);  // Same model, shared
    node2->SetPosition(glm::vec3(5.0F, 0.0F, 0.0F));

    auto* node3 = m_Scene->CreateNode("Volleyball3");
    node3->SetModel(model);
    node3->SetPosition(glm::vec3(-5.0F, 0.0F, 0.0F));

    // Create a child node to demonstrate hierarchy
    auto* child = node1->CreateChild("ChildBall");
    child->SetModel(model);
    child->SetPosition(glm::vec3(0.0F, 3.0F, 0.0F));  // Offset from parent
    child->SetScale(0.5F);  // Half size

    // Update all transforms
    m_Scene->UpdateTransforms();

    // Setup camera
    m_Camera.SetPerspective(45.0F, 16.0F / 9.0F, 0.01F, 100.0F);
    m_Camera.SetPosition(glm::vec3(15.0F, 10.0F, 15.0F));
    m_Camera.SetTarget(glm::vec3(0.0F, 0.0F, 0.0F));

    // Create camera controllers
    m_OrbitController = std::make_unique<OrbitCameraController>(&m_Camera);
    m_OrbitController->SetTarget(glm::vec3(0.0F, 0.0F, 0.0F));
    m_OrbitController->SetDistance(20.0F);
    m_OrbitController->SetDistanceLimits(5.0F, 50.0F);

    m_FPSController = std::make_unique<FPSCameraController>(&m_Camera);
    m_FPSController->SetMoveSpeed(10.0F);

    m_ActiveController = m_OrbitController.get();

    Logger::Info("Scene created with {} nodes", m_Scene->GetNodeCount());
    Logger::Info("Controls: 1=Orbit camera, 2=FPS camera, ESC=Exit");
    Logger::Info("          R=Rotate node1, Space=Toggle child visibility");
  }

  void OnUpdate(float delta_time) override
  {
    // ESC to exit
    if (Input::IsKeyPressed(KeyCode::Escape)) {
      GetWindow().RequestClose();
      return;
    }

    // Camera switching
    if (Input::IsKeyPressed(KeyCode::Num1)) {
      m_ActiveController = m_OrbitController.get();
      Input::SetMouseCaptured(/*captured=*/false);
      Logger::Info("Switched to Orbit camera");
    }

    if (Input::IsKeyPressed(KeyCode::Num2)) {
      m_ActiveController = m_FPSController.get();
      Logger::Info("Switched to FPS camera");
    }

    // Rotate first node when R is held
    if (Input::IsKeyDown(KeyCode::R)) {
      auto* node = m_Scene->FindNode("Volleyball1");
      if (node != nullptr) {
        node->GetTransform().RotateEuler(
            glm::vec3(0.0F, 45.0F * delta_time, 0.0F));
      }
    }

    // Toggle child visibility with Space
    if (Input::IsKeyPressed(KeyCode::Space)) {
      auto* child = m_Scene->FindNode("ChildBall");
      if (child != nullptr) {
        child->SetVisible(!child->IsVisible());
        Logger::Info("Child visibility: {}", child->IsVisible() ? "on" : "off");
      }
    }

    m_ActiveController->Update(delta_time);

    // Update aspect ratio
    auto* swapchain = GetDevice().GetSwapchain();
    const float aspect = static_cast<float>(swapchain->GetWidth())
        / static_cast<float>(swapchain->GetHeight());
    m_Camera.SetAspectRatio(aspect);

    // Update scene transforms
    m_Scene->UpdateTransforms();
  }

  void OnRender(float /*delta_time*/) override
  {
    auto& device = GetDevice();
    auto* cmd = device.GetCurrentCommandBuffer();

    // Begin frame with current camera
    m_SceneRenderer->BeginFrame(m_Camera);

    // Render the scene
    m_SceneRenderer->RenderScene(*cmd, *m_Scene);
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
