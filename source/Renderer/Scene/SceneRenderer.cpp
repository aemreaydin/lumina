#include "Renderer/Scene/SceneRenderer.hpp"

#include "Renderer/Asset/AssetManager.hpp"
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
#include "Renderer/ShaderCompiler.hpp"

SceneRenderer::SceneRenderer(RHIDevice& device, AssetManager& asset_manager)
    : m_Device(device)
    , m_AssetManager(asset_manager)
{
  create_descriptor_layouts();
  create_pipeline_layout();
  create_shaders();
  create_camera_resources();
}

SceneRenderer::~SceneRenderer() = default;

void SceneRenderer::BeginFrame(const Camera& camera)
{
  update_camera_ubo(camera);
}

void SceneRenderer::RenderScene(RHICommandBuffer& cmd, const Scene& scene)
{
  // Bind shaders once for the entire scene
  cmd.BindShaders(m_VertexShader.get(), m_FragmentShader.get());
  cmd.SetPrimitiveTopology(PrimitiveTopology::TriangleList);

  // Set vertex input layout
  cmd.SetVertexInput(Vertex::GetLayout());

  // Bind camera descriptor set (set 0)
  cmd.BindDescriptorSet(0, *m_CameraDescriptorSet, *m_PipelineLayout);

  // Render all visible nodes with models
  auto renderable_nodes = scene.GetRenderableNodes();
  for (auto* node : renderable_nodes) {
    RenderNode(cmd, *node);
  }
}

void SceneRenderer::RenderNode(RHICommandBuffer& cmd, const SceneNode& node)
{
  auto model = node.GetModel();
  if (!model || !model->AreResourcesCreated()) {
    return;
  }

  // Update node UBO with this node's transform
  update_node_ubo(node);

  // Bind node descriptor set (set 1)
  cmd.BindDescriptorSet(1, *m_NodeDescriptorSet, *m_PipelineLayout);

  // Render each mesh in the model
  for (size_t mesh_idx = 0; mesh_idx < model->GetMeshCount(); ++mesh_idx) {
    auto* mesh = model->GetMesh(mesh_idx);
    if (mesh == nullptr) {
      continue;
    }

    // Bind mesh buffers
    cmd.BindVertexBuffer(*mesh->GetVertexBuffer(), 0);
    cmd.BindIndexBuffer(*mesh->GetIndexBuffer());

    // Render each submesh with its material
    for (size_t submesh_idx = 0; submesh_idx < mesh->GetSubMeshCount();
         ++submesh_idx)
    {
      const auto& submesh = mesh->GetSubMesh(submesh_idx);

      // Get material for this submesh
      auto* material = model->GetMaterial(submesh.MaterialIndex);
      if (material != nullptr && material->GetDescriptorSet() != nullptr) {
        // Bind material descriptor set (set 2)
        cmd.BindDescriptorSet(
            2, *material->GetDescriptorSet(), *m_PipelineLayout);
      }

      // Draw the submesh
      cmd.DrawIndexed(submesh.IndexCount,
                      1,
                      submesh.IndexOffset,
                      static_cast<int32_t>(submesh.VertexOffset),
                      0);
    }
  }
}

auto SceneRenderer::GetCameraDescriptorSetLayout() const
    -> std::shared_ptr<RHIDescriptorSetLayout>
{
  return m_CameraSetLayout;
}

auto SceneRenderer::GetNodeDescriptorSetLayout() const
    -> std::shared_ptr<RHIDescriptorSetLayout>
{
  return m_NodeSetLayout;
}

auto SceneRenderer::GetPipelineLayout() const
    -> std::shared_ptr<RHIPipelineLayout>
{
  return m_PipelineLayout;
}

void SceneRenderer::create_shaders()
{
  auto shader_sources = ShaderCompiler::Compile("shaders/scene.slang");

  const auto& vertex_spirv = shader_sources.at(ShaderType::Vertex);
  ShaderModuleDesc vertex_desc {};
  vertex_desc.Stage = ShaderStage::Vertex;
  vertex_desc.SPIRVCode = vertex_spirv;
  vertex_desc.EntryPoint = "vertexMain";
  vertex_desc.SetLayouts = {m_CameraSetLayout,
                            m_NodeSetLayout,
                            m_AssetManager.GetMaterialDescriptorSetLayout()};
  m_VertexShader = m_Device.CreateShaderModule(vertex_desc);

  const auto& fragment_spirv = shader_sources.at(ShaderType::Fragment);
  ShaderModuleDesc fragment_desc {};
  fragment_desc.Stage = ShaderStage::Fragment;
  fragment_desc.SPIRVCode = fragment_spirv;
  fragment_desc.EntryPoint = "fragmentMain";
  fragment_desc.SetLayouts = {m_CameraSetLayout,
                              m_NodeSetLayout,
                              m_AssetManager.GetMaterialDescriptorSetLayout()};
  m_FragmentShader = m_Device.CreateShaderModule(fragment_desc);
}

void SceneRenderer::create_descriptor_layouts()
{
  // Set 0: Camera data
  DescriptorSetLayoutDesc camera_layout_desc {};
  camera_layout_desc.Bindings = {
      {0, DescriptorType::UniformBuffer, ShaderStage::Vertex, 1},
  };
  m_CameraSetLayout = m_Device.CreateDescriptorSetLayout(camera_layout_desc);

  // Set 1: Node/Transform data
  DescriptorSetLayoutDesc node_layout_desc {};
  node_layout_desc.Bindings = {
      {0, DescriptorType::UniformBuffer, ShaderStage::Vertex, 1},
  };
  m_NodeSetLayout = m_Device.CreateDescriptorSetLayout(node_layout_desc);
}

void SceneRenderer::create_pipeline_layout()
{
  PipelineLayoutDesc layout_desc {};
  layout_desc.SetLayouts = {m_CameraSetLayout,
                            m_NodeSetLayout,
                            m_AssetManager.GetMaterialDescriptorSetLayout()};
  m_PipelineLayout = m_Device.CreatePipelineLayout(layout_desc);
}

void SceneRenderer::create_camera_resources()
{
  // Camera UBO
  BufferDesc camera_buffer_desc {};
  camera_buffer_desc.Size = sizeof(CameraUBO);
  camera_buffer_desc.Usage = BufferUsage::Uniform;
  camera_buffer_desc.CPUVisible = true;
  m_CameraUBO = m_Device.CreateBuffer(camera_buffer_desc);

  // Camera descriptor set
  m_CameraDescriptorSet = m_Device.CreateDescriptorSet(m_CameraSetLayout);
  m_CameraDescriptorSet->WriteBuffer(
      0, m_CameraUBO.get(), 0, sizeof(CameraUBO));

  // Node UBO (reused for each node)
  BufferDesc node_buffer_desc {};
  node_buffer_desc.Size = sizeof(NodeUBO);
  node_buffer_desc.Usage = BufferUsage::Uniform;
  node_buffer_desc.CPUVisible = true;
  m_NodeUBO = m_Device.CreateBuffer(node_buffer_desc);

  // Node descriptor set
  m_NodeDescriptorSet = m_Device.CreateDescriptorSet(m_NodeSetLayout);
  m_NodeDescriptorSet->WriteBuffer(0, m_NodeUBO.get(), 0, sizeof(NodeUBO));
}

void SceneRenderer::update_camera_ubo(const Camera& camera)
{
  CameraUBO data {};
  data.View = camera.GetViewMatrix();
  data.Projection = camera.GetProjectionMatrix();
  data.ViewProjection = camera.GetViewProjectionMatrix();
  data.CameraPosition = glm::vec4(camera.GetPosition(), 1.0F);

  m_CameraUBO->Upload(&data, sizeof(CameraUBO), 0);
}

void SceneRenderer::update_node_ubo(const SceneNode& node)
{
  NodeUBO data {};
  data.Model = node.GetTransform().GetWorldMatrix();
  // Store normal matrix as mat4 for alignment (shader uses mat3 portion)
  glm::mat3 normal_mat = node.GetTransform().GetNormalMatrix();
  data.NormalMatrix = glm::mat4(normal_mat);

  m_NodeUBO->Upload(&data, sizeof(NodeUBO), 0);
}
