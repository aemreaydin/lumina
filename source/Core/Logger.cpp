#include "Core/Logger.hpp"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

std::shared_ptr<spdlog::logger> Logger::SLogger;

void Logger::Init()
{
  spdlog::set_pattern("%^[%T] [%l] %v%$");

  SLogger = spdlog::stdout_color_mt("LUMINA");
  SLogger->set_level(spdlog::level::trace);

  Logger::Info("Logging system initialized");
}
