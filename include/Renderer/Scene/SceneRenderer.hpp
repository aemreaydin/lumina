#ifndef RENDERER_SCENE_SCENERENDERER_HPP
#define RENDERER_SCENE_SCENERENDERER_HPP

#include <memory>

#include <glm/glm.hpp>

#include "Renderer/RHI/RHIPipeline.hpp"
#include "Renderer/ShaderCompiler.hpp"
#include "Renderer/ShaderReflection.hpp"

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

struct CameraUBO
{
  glm::mat4 View;
  glm::mat4 Projection;
  glm::mat4 ViewProjection;
  glm::vec4 CameraPosition;
};

struct NodeUBO
{
  glm::mat4 Model;
  glm::mat4 NormalMatrix;
};

class SceneRenderer
{
public:
  explicit SceneRenderer(RHIDevice& device);
  ~SceneRenderer();

  SceneRenderer(const SceneRenderer&) = delete;
  SceneRenderer(SceneRenderer&&) = delete;
  auto operator=(const SceneRenderer&) -> SceneRenderer& = delete;
  auto operator=(SceneRenderer&&) -> SceneRenderer& = delete;

  void BeginFrame(const Camera& camera);
  void RenderScene(RHICommandBuffer& cmd, const Scene& scene);
  void RenderNode(RHICommandBuffer& cmd, const SceneNode& node);

  [[nodiscard]] auto GetSetLayout(const std::string& parameter_name) const
      -> std::shared_ptr<RHIDescriptorSetLayout>;
  [[nodiscard]] auto GetPipelineLayout() const
      -> std::shared_ptr<RHIPipelineLayout>;

private:
  void compile_and_reflect();
  void create_pipeline_layout();
  void create_shaders();
  void create_camera_resources();
  void update_camera_ubo(const Camera& camera);

  RHIDevice& m_Device;

  ShaderCompileResult m_CompileResult;
  ReflectedPipelineLayout m_ReflectedLayout;

  std::unique_ptr<RHIShaderModule> m_VertexShader;
  std::unique_ptr<RHIShaderModule> m_FragmentShader;

  std::shared_ptr<RHIPipelineLayout> m_PipelineLayout;

  std::unique_ptr<RHIBuffer> m_CameraUBO;
  std::unique_ptr<RHIDescriptorSet> m_CameraDescriptorSet;

  std::unique_ptr<RHIBuffer> m_NodeDynamicBuffer;
  std::unique_ptr<RHIDescriptorSet> m_NodeDescriptorSet;
  uint32_t m_NodeSetIndex {0};
  uint32_t m_NodeDynamicOffset {0};
  uint32_t m_NodeAlignment {256};
};

#endif
