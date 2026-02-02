#include "Renderer/Scene/SceneRenderer.hpp"

#include "Core/Logger.hpp"
#include "Renderer/Camera.hpp"
#include "Renderer/Model/Material.hpp"
#include "Renderer/Model/Mesh.hpp"
#include "Renderer/Model/Model.hpp"
#include "Renderer/Model/Vertex.hpp"
#include "Renderer/RHI/RHIBuffer.hpp"
#include "Renderer/RHI/RHICommandBuffer.hpp"
#include "Renderer/RHI/RHIDescriptorSet.hpp"
#include "Renderer/RHI/RHIDevice.hpp"
#include "Renderer/RHI/RHIPipeline.hpp"
#include "Renderer/RHI/RHIShaderModule.hpp"
#include "Renderer/Scene/Scene.hpp"
#include "Renderer/Scene/SceneNode.hpp"

SceneRenderer::SceneRenderer(RHIDevice& device)
    : m_Device(device)
{
  compile_and_reflect();
  create_pipeline_layout();
  create_shaders();
  create_camera_resources();
}

SceneRenderer::~SceneRenderer() = default;

void SceneRenderer::BeginFrame(const Camera& camera)
{
  update_camera_ubo(camera);
  m_NodeDynamicOffset = 0;
}

void SceneRenderer::SetWireframe(bool wireframe)
{
  m_Wireframe = wireframe;
}

void SceneRenderer::RenderScene(RHICommandBuffer& cmd, const Scene& scene)
{
  cmd.SetPrimitiveTopology(PrimitiveTopology::TriangleList);
  cmd.SetPolygonMode(m_Wireframe ? PolygonMode::Line : PolygonMode::Fill);
  cmd.BindShaders(m_VertexShader.get(), m_FragmentShader.get());

  cmd.SetVertexInput(Vertex::GetLayout());

  cmd.BindDescriptorSet(m_ReflectedLayout.GetSetIndex("camera"),
                        *m_CameraDescriptorSet,
                        *m_PipelineLayout);

  auto renderable_nodes = scene.GetRenderableNodes();
  for (auto* node : renderable_nodes) {
    RenderNode(cmd, *node);
  }

  // Reset to fill so ImGui renders normally
  // cmd.SetPolygonMode(PolygonMode::Fill);
}

void SceneRenderer::RenderNode(RHICommandBuffer& cmd, const SceneNode& node)
{
  auto model = node.GetModel();
  if (!model || !model->AreResourcesCreated()) {
    return;
  }

  NodeUBO data {};
  data.Model = node.GetTransform().GetWorldMatrix();
  const auto normal_mat = node.GetTransform().GetNormalMatrix();
  data.NormalMatrix = linalg::Mat4 {normal_mat(0, 0),
                                    normal_mat(0, 1),
                                    normal_mat(0, 2),
                                    0.0F,
                                    normal_mat(1, 0),
                                    normal_mat(1, 1),
                                    normal_mat(1, 2),
                                    0.0F,
                                    normal_mat(2, 0),
                                    normal_mat(2, 1),
                                    normal_mat(2, 2),
                                    0.0F,
                                    0.0F,
                                    0.0F,
                                    0.0F,
                                    1.0F};

  m_NodeDynamicBuffer->Upload(&data, sizeof(NodeUBO), m_NodeDynamicOffset);

  uint32_t offsets[] = {m_NodeDynamicOffset};
  cmd.BindDescriptorSet(
      m_NodeSetIndex, *m_NodeDescriptorSet, *m_PipelineLayout, offsets);

  m_NodeDynamicOffset =
      (m_NodeDynamicOffset + sizeof(NodeUBO) + m_NodeAlignment - 1)
      & ~(m_NodeAlignment - 1);

  for (size_t mesh_idx = 0; mesh_idx < model->GetMeshCount(); ++mesh_idx) {
    auto* mesh = model->GetMesh(mesh_idx);
    if (mesh == nullptr) {
      continue;
    }

    cmd.BindVertexBuffer(*mesh->GetVertexBuffer(), 0);
    cmd.BindIndexBuffer(*mesh->GetIndexBuffer());

    for (size_t submesh_idx = 0; submesh_idx < mesh->GetSubMeshCount();
         ++submesh_idx)
    {
      const auto& submesh = mesh->GetSubMesh(submesh_idx);

      auto* material = model->GetMaterial(submesh.MaterialIndex);
      if (material != nullptr && material->GetDescriptorSet() != nullptr) {
        cmd.BindDescriptorSet(m_ReflectedLayout.GetSetIndex("material"),
                              *material->GetDescriptorSet(),
                              *m_PipelineLayout);
      }

      cmd.DrawIndexed(submesh.IndexCount,
                      1,
                      submesh.IndexOffset,
                      static_cast<int32_t>(submesh.VertexOffset),
                      0);
    }
  }
}

auto SceneRenderer::GetSetLayout(const std::string& parameter_name) const
    -> std::shared_ptr<RHIDescriptorSetLayout>
{
  return m_ReflectedLayout.GetSetLayout(parameter_name);
}

auto SceneRenderer::GetPipelineLayout() const
    -> std::shared_ptr<RHIPipelineLayout>
{
  return m_PipelineLayout;
}

void SceneRenderer::compile_and_reflect()
{
  m_CompileResult = ShaderCompiler::Compile("shaders/scene.slang");

  Logger::Info("Shader compiled with {} descriptor sets",
               m_CompileResult.Reflection.DescriptorSets.size());
}

void SceneRenderer::create_pipeline_layout()
{
  m_ReflectedLayout =
      CreatePipelineLayoutFromReflection(m_Device, m_CompileResult.Reflection);

  m_PipelineLayout =
      m_Device.CreatePipelineLayout(m_ReflectedLayout.SetLayouts);
}

void SceneRenderer::create_shaders()
{
  const auto& vertex_spirv = m_CompileResult.Sources.at(ShaderType::Vertex);
  ShaderModuleDesc vertex_desc {};
  vertex_desc.Stage = ShaderStage::Vertex;
  vertex_desc.SPIRVCode = vertex_spirv;
  vertex_desc.EntryPoint = "vertexMain";
  vertex_desc.SetLayouts = m_ReflectedLayout.SetLayouts;
  m_VertexShader = m_Device.CreateShaderModule(vertex_desc);

  const auto& fragment_spirv = m_CompileResult.Sources.at(ShaderType::Fragment);
  ShaderModuleDesc fragment_desc {};
  fragment_desc.Stage = ShaderStage::Fragment;
  fragment_desc.SPIRVCode = fragment_spirv;
  fragment_desc.EntryPoint = "fragmentMain";
  fragment_desc.SetLayouts = m_ReflectedLayout.SetLayouts;
  m_FragmentShader = m_Device.CreateShaderModule(fragment_desc);
}

void SceneRenderer::create_camera_resources()
{
  BufferDesc camera_buffer_desc {};
  camera_buffer_desc.Size = sizeof(CameraUBO);
  camera_buffer_desc.Usage = BufferUsage::Uniform;
  camera_buffer_desc.CPUVisible = true;
  m_CameraUBO = m_Device.CreateBuffer(camera_buffer_desc);

  auto camera_layout = m_ReflectedLayout.GetSetLayout("camera");
  m_CameraDescriptorSet = m_Device.CreateDescriptorSet(camera_layout);
  m_CameraDescriptorSet->WriteBuffer(
      0, m_CameraUBO.get(), 0, sizeof(CameraUBO));

  BufferDesc node_desc {};
  node_desc.Size = 1024 * m_NodeAlignment;
  node_desc.Usage = BufferUsage::Uniform;
  node_desc.CPUVisible = true;
  m_NodeDynamicBuffer = m_Device.CreateBuffer(node_desc);

  m_NodeSetIndex = m_ReflectedLayout.GetSetIndex("node");
  auto node_layout = m_ReflectedLayout.GetSetLayout("node");
  m_NodeDescriptorSet = m_Device.CreateDescriptorSet(node_layout);
  m_NodeDescriptorSet->WriteBuffer(
      0, m_NodeDynamicBuffer.get(), 0, sizeof(NodeUBO));
}

void SceneRenderer::update_camera_ubo(const Camera& camera)
{
  CameraUBO data {};
  data.View = camera.GetViewMatrix();
  data.Projection = camera.GetProjectionMatrix();
  data.ViewProjection = camera.GetViewProjectionMatrix();
  data.CameraPosition = linalg::Vec4(camera.GetPosition(), 1.0F);

  m_CameraUBO->Upload(&data, sizeof(CameraUBO), 0);
}
