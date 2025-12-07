#ifndef CORE_LOGGER_HPP
#define CORE_LOGGER_HPP

#include <memory>

#include <spdlog/common.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

struct LoggerConfig
{
  spdlog::level::level_enum Level {spdlog::level::trace};
};

class Logger

{
public:
  static void Init(const LoggerConfig& logger_config);

  static auto GetLogger() -> std::shared_ptr<spdlog::logger>&
  {
    return SLogger;
  }

  template<typename... Args>
  static void Trace(spdlog::format_string_t<Args...> fmt, Args&&... args)
  {
    SLogger->trace(fmt, std::forward<Args>(args)...);
  }

  template<typename... Args>
  static void Info(spdlog::format_string_t<Args...> fmt, Args&&... args)
  {
    SLogger->info(fmt, std::forward<Args>(args)...);
  }

  template<typename... Args>
  static void Warn(spdlog::format_string_t<Args...> fmt, Args&&... args)
  {
    SLogger->warn(fmt, std::forward<Args>(args)...);
  }

  template<typename... Args>
  static void Error(spdlog::format_string_t<Args...> fmt, Args&&... args)
  {
    SLogger->error(fmt, std::forward<Args>(args)...);
  }

  template<typename... Args>
  static void Critical(spdlog::format_string_t<Args...> fmt, Args&&... args)
  {
    SLogger->critical(fmt, std::forward<Args>(args)...);
  }

private:
  static std::shared_ptr<spdlog::logger> SLogger;
};

#endif
