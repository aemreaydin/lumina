#ifndef CORE_CONFIGLOADER_HPP
#define CORE_CONFIGLOADER_HPP

#include <filesystem>

#include "Renderer/RendererConfig.hpp"

class ConfigLoader
{
public:
  static auto LoadRendererConfig(const std::filesystem::path& config_path)
      -> RendererConfig;

  static void SaveRendererConfig(const RendererConfig& config,
                                 const std::filesystem::path& config_path);

  static auto CreateDefaultConfig() -> RendererConfig;
};

#endif
