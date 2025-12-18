#include <span>

#include "Renderer/ShaderCompiler.hpp"

#include <slang-com-ptr.h>
#include <slang.h>

#include "Core/Logger.hpp"

using Slang::ComPtr;

static auto CompileStage(slang::ISession* session,
                         slang::IModule* module,
                         const char* entry_point_name)
    -> std::optional<std::vector<uint32_t>>
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
  Slang::ComPtr<slang::IBlob> json;
  program_layout->toJson(json.writeRef());

  const auto entry_point_count = program_layout->getEntryPointCount();
  Logger::Info("Entry Point Count: {}", entry_point_count);

  slang::EntryPointLayout* entry_point_layout =
      program_layout->getEntryPointByIndex(0);
  Logger::Info("Entry Point Name: {}", entry_point_layout->getName());

  ComPtr<slang::IBlob> spirv_blob;
  linked_program->getEntryPointCode(
      0, 0, spirv_blob.writeRef(), diagnostics.writeRef());
  if (diagnostics != nullptr) {
    Logger::Critical("Failed to get entry point code");
    throw std::runtime_error("slang getEntryPointCode failed.");
  }

  const auto* ptr =
      static_cast<const uint32_t*>(spirv_blob->getBufferPointer());
  size_t const count = spirv_blob->getBufferSize() / sizeof(uint32_t);
  const auto view = std::span {ptr, count};
  return std::vector<uint32_t> {view.begin(), view.end()};
}

auto ShaderCompiler::Compile(const std::string& shader_path) -> ShaderSources
{
  ShaderSources sources;

  ComPtr<slang::IGlobalSession> global_session;
  slang::createGlobalSession(global_session.writeRef());

  slang::TargetDesc const target_desc = {
      .format = SLANG_SPIRV,
      .profile = global_session->findProfile("glsl_460"),
      .forceGLSLScalarBufferLayout = true};

  // Preserve entry point names in SPIR-V output
  slang::CompilerOptionEntry option {};
  option.name = slang::CompilerOptionName::VulkanUseEntryPointName;
  option.value.kind = slang::CompilerOptionValueKind::Int;
  option.value.intValue0 = 1;

  slang::SessionDesc const session_desc = {.targets = &target_desc,
                                           .targetCount = 1,
                                           .compilerOptionEntries = &option,
                                           .compilerOptionEntryCount = 1};

  ComPtr<slang::ISession> session;
  global_session->createSession(session_desc, session.writeRef());

  Slang::ComPtr<slang::IBlob> diagnostics;
  Slang::ComPtr<slang::IModule> const module(
      session->loadModule(shader_path.c_str(), diagnostics.writeRef()));
  if (diagnostics != nullptr) {
    Logger::Critical("Failed to load slang module");
    throw std::runtime_error("slang loadModule failed.");
  }

  const auto vertex_res = CompileStage(session, module, "vertexMain");
  if (vertex_res.has_value()) {
    sources.emplace(ShaderType::Vertex, vertex_res.value());
  }

  const auto fragment_res = CompileStage(session, module, "fragmentMain");
  if (fragment_res.has_value()) {
    sources.emplace(ShaderType::Fragment, fragment_res.value());
  }

  const auto compute_res = CompileStage(session, module, "computeMain");
  if (compute_res.has_value()) {
    sources.emplace(ShaderType::Compute, compute_res.value());
  }

  return sources;
}
