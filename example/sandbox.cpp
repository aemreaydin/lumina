#include <memory>

#include "Core/Application.hpp"
#include "Core/Logger.hpp"

class SandboxApp : public Application
{
public:
  SandboxApp() { Logger::Info("Sandbox App Initialized"); }
};

auto CreateApplication() -> std::unique_ptr<Application>
{
  return std::make_unique<SandboxApp>();
}

auto main() -> int
{
  const auto app = CreateApplication();
  app->Run();

  Logger::Info("Application shutting down");
  return 0;
}
