#include <filesystem>
#include <iostream>
#include <vector>

#include "Core/Logger.hpp"

#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

std::shared_ptr<spdlog::logger> Logger::SLogger;

void Logger::Init(const LoggerConfig& logger_config)
{
  try {
    // Create logs directory if it doesn't exist
    std::filesystem::create_directories("logs");

    // Create sinks
    std::vector<spdlog::sink_ptr> sinks;

    // Console sink (with colors)
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_pattern("%^[%T] [%l] %v%$");
    sinks.push_back(console_sink);

    // Daily file sink (rotates at midnight)
    auto file_sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>(
        "logs/lumina.log", 0, 0, true);  // Rotate at midnight (00:00)
    file_sink->set_pattern("[%Y-%m-%d %T] [%l] %v");
    sinks.push_back(file_sink);

    // Create logger with both sinks
    SLogger =
        std::make_shared<spdlog::logger>("LUMINA", sinks.begin(), sinks.end());
    SLogger->set_level(logger_config.Level);
    SLogger->flush_on(spdlog::level::err);

    Logger::Info("Logging system initialized");
  } catch (const spdlog::spdlog_ex& ex) {
    std::cerr << "Log initialization failed: " << ex.what() << '\n';
  }
}
