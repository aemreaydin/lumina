#include <map>
#include <span>

#include "Renderer/ShaderCompiler.hpp"

#include <slang-com-ptr.h>
#include <slang.h>
#include <spirv_cross/spirv_glsl.hpp>

#include "Core/Logger.hpp"

using Slang::ComPtr;

static constexpr uint32_t kGLBindingStride = 16;

static auto SpirvToGLSL(const std::vector<uint32_t>& spirv) -> std::string
{
  spirv_cross::CompilerGLSL compiler(spirv);

  spirv_cross::CompilerGLSL::Options opts;
  opts.version = 460;
  opts.es = false;
  opts.vulkan_semantics = false;
  opts.enable_420pack_extension = false;
  compiler.set_common_options(opts);

  auto remap =
      [&](const spirv_cross::SmallVector<spirv_cross::Resource>& resources)
  {
    for (const auto& res : resources) {
      uint32_t set =
          compiler.get_decoration(res.id, spv::DecorationDescriptorSet);
      uint32_t binding =
          compiler.get_decoration(res.id, spv::DecorationBinding);
      uint32_t flat = set * kGLBindingStride + binding;
      compiler.set_decoration(res.id, spv::DecorationDescriptorSet, 0);
      compiler.set_decoration(res.id, spv::DecorationBinding, flat);
    }
  };

  auto resources = compiler.get_shader_resources();
  remap(resources.uniform_buffers);
  remap(resources.storage_buffers);
  remap(resources.sampled_images);
  remap(resources.separate_images);
  remap(resources.separate_samplers);
  remap(resources.storage_images);

  // Merge separate image + sampler pairs into combined image samplers
  compiler.build_combined_image_samplers();

  // Remap combined image sampler bindings to match the image binding
  for (auto& combined : compiler.get_combined_image_samplers()) {
    uint32_t binding =
        compiler.get_decoration(combined.image_id, spv::DecorationBinding);
    compiler.set_decoration(
        combined.combined_id, spv::DecorationBinding, binding);
  }

  return compiler.compile();
}

static auto MapBindingType(slang::BindingType type) -> ShaderParameterType
{
  switch (type) {
    case slang::BindingType::ConstantBuffer:
      return ShaderParameterType::UniformBuffer;
    case slang::BindingType::CombinedTextureSampler:
      return ShaderParameterType::CombinedImageSampler;
    case slang::BindingType::Texture:
      return ShaderParameterType::SampledImage;
    case slang::BindingType::Sampler:
      return ShaderParameterType::Sampler;
    case slang::BindingType::RawBuffer:
    case slang::BindingType::TypedBuffer:
      return ShaderParameterType::StorageBuffer;
    default:
      return ShaderParameterType::UniformBuffer;
  }
}

static void ReflectParameterBlock(
    slang::VariableLayoutReflection* param,
    bool is_dynamic,
    std::map<uint32_t, ShaderDescriptorSetInfo>& set_map)
{
  auto* type_layout = param->getTypeLayout();
  auto set = static_cast<uint32_t>(
      param->getOffset(slang::ParameterCategory::SubElementRegisterSpace));

  auto& set_info = set_map[set];
  set_info.SetIndex = set;
  set_info.BlockName = param->getName();

  auto* element_type = type_layout->getElementTypeLayout();
  if (element_type == nullptr) {
    return;
  }

  auto data_size = element_type->getSize();
  if (data_size > 0) {
    ShaderParameterInfo info;
    info.Name = param->getName();
    info.Type = is_dynamic ? ShaderParameterType::DynamicUniformBuffer
                           : ShaderParameterType::UniformBuffer;
    info.Set = set;
    info.Binding = 0;
    info.Size = static_cast<uint32_t>(data_size);
    info.Count = 1;
    info.Stages = ShaderStage::Vertex | ShaderStage::Fragment;
    set_info.Parameters.push_back(info);
  }

  auto range_count = element_type->getBindingRangeCount();
  for (SlangInt r = 0; r < range_count; r++) {
    auto binding_type = element_type->getBindingRangeType(r);
    auto binding_count = element_type->getBindingRangeBindingCount(r);

    if (binding_type == slang::BindingType::ParameterBlock
        || binding_type == slang::BindingType::ConstantBuffer
        || binding_type == slang::BindingType::InlineUniformData)
    {
      continue;
    }

    auto* leaf_var = element_type->getBindingRangeLeafVariable(r);
    std::string range_name =
        leaf_var != nullptr ? leaf_var->getName() : "unknown";

    auto desc_set_idx = element_type->getBindingRangeDescriptorSetIndex(r);
    auto first_range_idx =
        element_type->getBindingRangeFirstDescriptorRangeIndex(r);
    auto binding_offset =
        element_type->getDescriptorSetDescriptorRangeIndexOffset(
            desc_set_idx, first_range_idx);

    ShaderParameterInfo info;
    info.Name = range_name;
    info.Type = MapBindingType(binding_type);
    info.Set = set;
    info.Binding = static_cast<uint32_t>(binding_offset);
    info.Count = static_cast<uint32_t>(binding_count);
    info.Stages = ShaderStage::Vertex | ShaderStage::Fragment;
    set_info.Parameters.push_back(info);
  }
}

static auto ExtractReflection(slang::ProgramLayout* layout,
                              slang::IGlobalSession* global_session,
                              const std::string& source_path)
    -> ShaderReflectionData
{
  ShaderReflectionData result;
  result.SourcePath = source_path;

  std::map<uint32_t, ShaderDescriptorSetInfo> set_map;

  const auto param_count = layout->getParameterCount();
  for (unsigned i = 0; i < param_count; i++) {
    auto* param = layout->getParameterByIndex(i);
    auto* type_layout = param->getTypeLayout();
    auto kind = type_layout->getKind();
    auto category = param->getCategory();
    const char* name = param->getName();

    auto* variable = param->getVariable();
    bool is_dynamic =
        variable->findUserAttributeByName(
            reinterpret_cast<SlangSession*>(global_session), "Dynamic")
        != nullptr;

    if (category == slang::ParameterCategory::PushConstantBuffer) {
      auto set = static_cast<uint32_t>(param->getBindingSpace());
      auto binding = static_cast<uint32_t>(param->getBindingIndex());
      auto* element_type = type_layout->getElementTypeLayout();
      auto size = element_type != nullptr ? element_type->getSize()
                                          : type_layout->getSize();

      auto& set_info = set_map[set];
      set_info.SetIndex = set;

      ShaderParameterInfo info;
      info.Name = name;
      info.Type = is_dynamic ? ShaderParameterType::DynamicUniformBuffer
                             : ShaderParameterType::UniformBuffer;
      info.Set = set;
      info.Binding = binding;
      info.Size = static_cast<uint32_t>(size);
      info.Count = 1;
      info.Stages = ShaderStage::Vertex | ShaderStage::Fragment;
      set_info.Parameters.push_back(info);

      Logger::Info("  Reflection: ConstantBuffer (was push_constant) '{}' "
                   "set={} binding={} size={}",
                   name,
                   set,
                   binding,
                   size);
      continue;
    }

    if (kind == slang::TypeReflection::Kind::ParameterBlock) {
      Logger::Info("  Reflection: ParameterBlock '{}'{}",
                   name,
                   is_dynamic ? " [Dynamic]" : "");
      ReflectParameterBlock(param, is_dynamic, set_map);
      continue;
    }

    if (kind == slang::TypeReflection::Kind::ConstantBuffer) {
      auto set = static_cast<uint32_t>(param->getBindingSpace());
      auto binding = static_cast<uint32_t>(param->getBindingIndex());
      auto* element_type = type_layout->getElementTypeLayout();
      auto size = element_type != nullptr ? element_type->getSize()
                                          : type_layout->getSize();

      auto& set_info = set_map[set];
      set_info.SetIndex = set;

      ShaderParameterInfo info;
      info.Name = name;
      info.Type = is_dynamic ? ShaderParameterType::DynamicUniformBuffer
                             : ShaderParameterType::UniformBuffer;
      info.Set = set;
      info.Binding = binding;
      info.Size = static_cast<uint32_t>(size);
      info.Count = 1;
      info.Stages = ShaderStage::Vertex | ShaderStage::Fragment;
      set_info.Parameters.push_back(info);

      Logger::Info(
          "  Reflection: ConstantBuffer '{}' set={} binding={} " "size={}",
          name,
          set,
          binding,
          size);
      continue;
    }

    if (kind == slang::TypeReflection::Kind::Resource
        || kind == slang::TypeReflection::Kind::SamplerState)
    {
      auto set = static_cast<uint32_t>(param->getBindingSpace());
      auto binding = static_cast<uint32_t>(param->getBindingIndex());

      auto param_type = ShaderParameterType::CombinedImageSampler;
      if (type_layout->getBindingRangeCount() > 0) {
        param_type = MapBindingType(type_layout->getBindingRangeType(0));
      }

      auto& set_info = set_map[set];
      set_info.SetIndex = set;

      ShaderParameterInfo info;
      info.Name = name;
      info.Type = param_type;
      info.Set = set;
      info.Binding = binding;
      info.Size = 0;
      info.Count = 1;
      info.Stages = ShaderStage::Fragment;
      set_info.Parameters.push_back(info);

      Logger::Info(
          "  Reflection: Resource '{}' set={} binding={}", name, set, binding);
      continue;
    }

    Logger::Warn("  Reflection: unhandled parameter '{}' kind={}",
                 name,
                 static_cast<int>(kind));
  }

  for (auto& [idx, set_info] : set_map) {
    Logger::Info("  Reflection: descriptor set {} with {} bindings",
                 idx,
                 set_info.Parameters.size());
    result.DescriptorSets.push_back(std::move(set_info));
  }

  return result;
}

struct StageCompileResult
{
  std::vector<uint32_t> SPIRV;
  ShaderReflectionData Reflection;
};

static auto CompileStage(slang::ISession* session,
                         slang::IGlobalSession* global_session,
                         slang::IModule* module,
                         const char* entry_point_name,
                         const std::string& source_path)
    -> std::optional<StageCompileResult>
{
  ComPtr<slang::IEntryPoint> entry_point;
  module->findEntryPointByName(entry_point_name, entry_point.writeRef());

  if (entry_point == nullptr) {
    return std::nullopt;
  }

  std::vector<slang::IComponentType*> components;
  components.emplace_back(module);
  components.emplace_back(entry_point);

  Slang::ComPtr<slang::IBlob> diagnostics;
  ComPtr<slang::IComponentType> program;
  session->createCompositeComponentType(
      components.data(),
      static_cast<SlangInt>(components.size()),
      program.writeRef(),
      diagnostics.writeRef());
  if (diagnostics != nullptr) {
    Logger::Critical("Failed to create composite component type");
    throw std::runtime_error("slang createCompositeComponentType failed.");
  }

  ComPtr<slang::IComponentType> linked_program;
  program->link(linked_program.writeRef(), diagnostics.writeRef());
  if (diagnostics != nullptr) {
    Logger::Critical("Failed to link program");
    throw std::runtime_error("slang link failed.");
  }

  slang::ProgramLayout* program_layout = program->getLayout(0);

  Logger::Info("Extracting reflection for entry point '{}'", entry_point_name);
  auto reflection =
      ExtractReflection(program_layout, global_session, source_path);

  // Get SPIRV (target 0)
  ComPtr<slang::IBlob> spirv_blob;
  linked_program->getEntryPointCode(
      0, 0, spirv_blob.writeRef(), diagnostics.writeRef());
  if (spirv_blob == nullptr) {
    Logger::Critical("Failed to get entry point code");
    Logger::Critical("{}",
                     static_cast<const char*>(diagnostics->getBufferPointer()));
    throw std::runtime_error("slang getEntryPointCode failed.");
  }

  const auto* ptr =
      static_cast<const uint32_t*>(spirv_blob->getBufferPointer());
  size_t const count = spirv_blob->getBufferSize() / sizeof(uint32_t);
  const auto view = std::span {ptr, count};

  StageCompileResult result;
  result.SPIRV = {view.begin(), view.end()};
  result.Reflection = std::move(reflection);

  return result;
}

auto ShaderCompiler::Compile(const std::string& shader_path, RenderAPI api)
    -> ShaderCompileResult
{
  ShaderCompileResult result;

  ComPtr<slang::IGlobalSession> global_session;
  slang::createGlobalSession(global_session.writeRef());

  auto profile = global_session->findProfile("glsl_460");

  slang::TargetDesc spirv_target {
      .format = SLANG_SPIRV,
      .profile = profile,
  };

  slang::CompilerOptionEntry option {};
  option.name = slang::CompilerOptionName::VulkanUseEntryPointName;
  option.value.kind = slang::CompilerOptionValueKind::Int;
  option.value.intValue0 = 1;

  std::vector<slang::PreprocessorMacroDesc> macros;
  if (api == RenderAPI::Vulkan) {
    macros.push_back({.name = "VULKAN", .value = "1"});
  }
  slang::SessionDesc const session_desc = {
      .targets = &spirv_target,
      .targetCount = 1,
      .defaultMatrixLayoutMode =
          SlangMatrixLayoutMode::SLANG_MATRIX_LAYOUT_COLUMN_MAJOR,
      .preprocessorMacros = macros.data(),
      .preprocessorMacroCount = static_cast<uint32_t>(macros.size()),
      .compilerOptionEntries = &option,
      .compilerOptionEntryCount = 1,
  };

  ComPtr<slang::ISession> session;
  global_session->createSession(session_desc, session.writeRef());

  Slang::ComPtr<slang::IBlob> diagnostics;
  Slang::ComPtr<slang::IModule> const module(
      session->loadModule(shader_path.c_str(), diagnostics.writeRef()));
  if (diagnostics != nullptr) {
    Logger::Critical("Failed to load slang module");
    throw std::runtime_error("slang loadModule failed.");
  }

  bool reflection_extracted = false;

  auto process_stage = [&](const char* entry_point_name, ShaderType type)
  {
    auto stage_res = CompileStage(
        session, global_session, module, entry_point_name, shader_path);
    if (!stage_res.has_value()) {
      return;
    }

    if (!reflection_extracted) {
      result.Reflection = stage_res->Reflection;
      reflection_extracted = true;
    }

    if (api == RenderAPI::Vulkan) {
      result.Sources.emplace(type, std::move(stage_res->SPIRV));
    } else {
      auto glsl = SpirvToGLSL(stage_res->SPIRV);
      Logger::Info("  SPIRV-Cross generated GLSL for '{}' ({} bytes)",
                   entry_point_name,
                   glsl.size());
      result.GLSLSources.emplace(type, std::move(glsl));
    }
  };

  process_stage("vertexMain", ShaderType::Vertex);
  process_stage("fragmentMain", ShaderType::Fragment);
  process_stage("computeMain", ShaderType::Compute);

  return result;
}
