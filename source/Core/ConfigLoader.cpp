#include <fstream>

#include "Core/ConfigLoader.hpp"

#include <spdlog/common.h>
#include <toml++/toml.h>

#include "Core/Logger.hpp"

auto ConfigLoader::LoadRendererConfig(const std::filesystem::path& config_path)
    -> RendererConfig
{
  RendererConfig config;

  try {
    // Check if file exists
    if (!std::filesystem::exists(config_path)) {
      Logger::Warn("Config file not found: {}, creating default config",
                   config_path.string());
      config = CreateDefaultConfig();
      SaveRendererConfig(config, config_path);
      return config;
    }

    // Parse TOML file
    auto toml_config = toml::parse_file(config_path.string());

    // Read renderer section
    if (const auto* renderer = toml_config["renderer"].as_table()) {
      // Read API
      if (const auto* api = renderer->get("api")) {
        std::string api_str = api->value_or("OpenGL");
        if (api_str == "Vulkan") {
          config.API = RenderAPI::Vulkan;
        } else if (api_str == "OpenGL") {
          config.API = RenderAPI::OpenGL;
        } else {
          Logger::Warn("Unknown API '{}', defaulting to OpenGL", api_str);
          config.API = RenderAPI::OpenGL;
        }
      }

      // Read validation
      if (const auto* validation = renderer->get("validation")) {
        config.EnableValidation = validation->value_or(true);
      }
    }

    Logger::Info("Loaded config from: {}", config_path.string());
    Logger::Info("  API: {}",
                 config.API == RenderAPI::Vulkan ? "Vulkan" : "OpenGL");
    Logger::Info("  Validation: {}",
                 config.EnableValidation ? "enabled" : "disabled");

  } catch (const toml::parse_error& err) {
    Logger::Error("Failed to parse config file: {}", err.description());
    Logger::Warn("Using default configuration");
    return CreateDefaultConfig();
  }

  return config;
}

auto ConfigLoader::LoadLoggerConfig(const std::filesystem::path& config_path)
    -> LoggerConfig
{
  LoggerConfig config;

  try {
    if (!std::filesystem::exists(config_path)) {
      Logger::Warn("Config file not found: {}, creating default config",
                   config_path.string());
      return config;
    }

    auto toml_config = toml::parse_file(config_path.string());
    if (const auto* logger = toml_config["logger"].as_table()) {
      if (const auto* level_node = logger->get("level")) {
        const spdlog::level::level_enum level =
            spdlog::level::from_str(level_node->value_or("trace"));
        ;
        config.Level = level;
      }
    }
  } catch (const toml::parse_error& err) {
    Logger::Error("Failed to parse config file: {}", err.description());
    Logger::Warn("Using default configuration");
    return config;
  }

  return config;
}

void ConfigLoader::SaveRendererConfig(const RendererConfig& config,
                                      const std::filesystem::path& config_path)
{
  toml::table toml_config;

  toml::table renderer;
  renderer.insert_or_assign(
      "api", config.API == RenderAPI::Vulkan ? "Vulkan" : "OpenGL");
  renderer.insert_or_assign("validation", config.EnableValidation);

  toml_config.insert_or_assign("renderer", renderer);

  // Write to file
  std::ofstream file(config_path);
  if (!file) {
    Logger::Error("Failed to open config file for writing: {}",
                  config_path.string());
    return;
  }

  file << toml_config;
  Logger::Info("Saved config to: {}", config_path.string());
}

auto ConfigLoader::CreateDefaultConfig() -> RendererConfig
{
  RendererConfig config;
  config.API = RenderAPI::OpenGL;
  config.EnableValidation = true;
  return config;
}
