#ifndef RENDERER_SCENE_SCENERENDERER_HPP
#define RENDERER_SCENE_SCENERENDERER_HPP

#include <memory>

#include <glm/glm.hpp>

class Scene;
class SceneNode;
class Camera;
class RHIDevice;
class RHIBuffer;
class RHIDescriptorSet;
class RHIDescriptorSetLayout;
class RHIPipelineLayout;
class RHIShaderModule;
class RHICommandBuffer;
class AssetManager;

struct CameraUBO
{
  glm::mat4 View;
  glm::mat4 Projection;
  glm::mat4 ViewProjection;
  glm::vec4 CameraPosition;  // w unused
};

struct NodeUBO
{
  glm::mat4 Model;
  glm::mat4 NormalMatrix;  // Stored as mat4 for alignment, use mat3 portion
};

class SceneRenderer
{
public:
  SceneRenderer(RHIDevice& device, AssetManager& asset_manager);
  ~SceneRenderer();

  SceneRenderer(const SceneRenderer&) = delete;
  SceneRenderer(SceneRenderer&&) = delete;
  auto operator=(const SceneRenderer&) -> SceneRenderer& = delete;
  auto operator=(SceneRenderer&&) -> SceneRenderer& = delete;

  // Call once before rendering to set up for current frame
  void BeginFrame(const Camera& camera);

  // Render a scene
  void RenderScene(RHICommandBuffer& cmd, const Scene& scene);

  // Render a single node (useful for custom rendering)
  void RenderNode(RHICommandBuffer& cmd, const SceneNode& node);

  // Get layouts for external use (e.g., creating compatible pipelines)
  [[nodiscard]] auto GetCameraDescriptorSetLayout() const
      -> std::shared_ptr<RHIDescriptorSetLayout>;
  [[nodiscard]] auto GetNodeDescriptorSetLayout() const
      -> std::shared_ptr<RHIDescriptorSetLayout>;
  [[nodiscard]] auto GetPipelineLayout() const
      -> std::shared_ptr<RHIPipelineLayout>;

private:
  void create_shaders();
  void create_descriptor_layouts();
  void create_pipeline_layout();
  void create_camera_resources();
  void update_camera_ubo(const Camera& camera);
  void update_node_ubo(const SceneNode& node);

  RHIDevice& m_Device;
  AssetManager& m_AssetManager;

  // Shaders
  std::unique_ptr<RHIShaderModule> m_VertexShader;
  std::unique_ptr<RHIShaderModule> m_FragmentShader;

  // Descriptor set layouts (Set 0 = Camera, Set 1 = Node, Set 2 = Material)
  std::shared_ptr<RHIDescriptorSetLayout> m_CameraSetLayout;
  std::shared_ptr<RHIDescriptorSetLayout> m_NodeSetLayout;
  std::shared_ptr<RHIPipelineLayout> m_PipelineLayout;

  // Per-frame camera resources
  std::unique_ptr<RHIBuffer> m_CameraUBO;
  std::unique_ptr<RHIDescriptorSet> m_CameraDescriptorSet;

  // Per-node resources (reused each frame)
  std::unique_ptr<RHIBuffer> m_NodeUBO;
  std::unique_ptr<RHIDescriptorSet> m_NodeDescriptorSet;
};

#endif
