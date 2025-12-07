# Lumina Game Engine - Project Context

## Project Overview

Lumina is a game engine with a focus on graphics, written in modern C++23. The project is in early development (v0.1.0) and features a clean architecture with platform abstraction layers, OpenGL rendering, and a comprehensive logging system.

**Key Information:**
- Language: C++23
- Build System: CMake 3.14+
- Package Manager: vcpkg
- Testing Framework: Catch2 v3
- Primary Platform: Linux (Windows support planned)
- Graphics API: OpenGL 4.6 (via GLAD)
- Windowing: GLFW3

## Project Structure

```
lumina/
├── include/                 # Public API headers (organized by module)
│   ├── Core/               # Core engine components (Application, Logger, Window)
│   ├── Platform/           # Platform-specific implementations
│   │   ├── Linux/          # Linux-specific code (GLFW-based)
│   │   └── Windows/        # Windows-specific code (planned)
│   └── Renderer/           # Rendering system (GraphicsContext, OpenGL, Shaders)
├── source/                  # Implementation files (mirrors include structure)
│   ├── Core/
│   ├── Platform/
│   └── Renderer/
├── example/                 # Example applications (sandbox)
├── test/                    # Unit tests (Catch2)
├── docs/                    # Documentation (Doxygen + m.css)
├── cmake/                   # CMake modules and scripts
└── .github/                 # CI/CD workflows
```

### Key Files
- `CMakeLists.txt` - Main build configuration
- `CMakePresets.json` - Build presets (dev, ci-linux, ci-darwin, ci-windows)
- `vcpkg.json` - Dependency management
- `Taskfile.yaml` - Task automation (build, test, format, tidy)
- `.clang-format` - Code formatting rules
- `.clang-tidy` - Static analysis configuration (warnings as errors)

## Architecture and Design Patterns

### Core Principles
1. **Platform Abstraction Layer (PAL)**: Abstract interfaces in `Core/` with platform-specific implementations in `Platform/`
2. **Factory Pattern**: Static factory methods create platform-specific instances (e.g., `Window::Create()`)
3. **Interface-Based Design**: Pure virtual interfaces with platform implementations
4. **Graphics API Abstraction**: Abstract `GraphicsContext` with concrete implementations (currently `OpenGLContext`)
5. **Application Framework**: Central `Application` class manages the main loop and window lifecycle

### Example Pattern
```cpp
// Core/Window.hpp - Abstract interface
class Window {
public:
    virtual void OnUpdate() = 0;
    static auto Create(const WindowProps& props) -> std::unique_ptr<Window>;
};

// Platform/Linux/LinuxWindow.hpp - Platform implementation
class LinuxWindow : public Window {
    void OnUpdate() override;
    GLFWwindow* m_Window {nullptr};
    std::unique_ptr<GraphicsContext> m_GraphicsContext;
};

// Core/Window.cpp - Factory implementation
auto Window::Create(const WindowProps& props) -> std::unique_ptr<Window> {
#ifdef __linux__
    return std::make_unique<LinuxWindow>(props);
#elifdef _WIN32
    return std::make_unique<WindowsWindow>(props);
#endif
}
```

### Application Entry Point Pattern
```cpp
// example/sandbox.cpp
class SandboxApp : public Application {
public:
    SandboxApp() { Logger::Info("Sandbox App Initialized"); }
};

auto CreateApplication() -> std::unique_ptr<Application> {
    return std::make_unique<SandboxApp>();
}

auto main() -> int {
    Logger::Init();
    const auto app = CreateApplication();
    app->Run();
    return 0;
}
```

## Coding Conventions

### Naming Conventions (strictly enforced by clang-tidy)

**CamelCase (PascalCase):**
- Classes, structs: `Application`, `WindowProps`, `LinuxWindow`
- Enums: `ShaderType`
- Enum constants: `Vertex`, `Fragment`, `Compute`
- Public methods: `GetWidth()`, `OnUpdate()`, `SetVSync()`
- Public members: `WindowProps::Title`, `WindowProps::Dimensions`
- Private/protected methods (lowercase exceptions noted below)
- Global functions: `CreateApplication()`
- Namespaces: (when added in future)

**snake_case (lowercase with underscores):**
- Local variables: `current_frame_time`, `native_win`
- Function parameters: `shader_path`, `v_sync`, `enabled`
- Local constants: `shader_path`
- Private/protected methods: (prefer snake_case per .clang-tidy)

**UPPER_CASE:**
- Macros: `WIDTH`, `HEIGHT`, `MAJOR_VER`, `MINOR_VER`
- Global constants: `constexpr uint32_t WIDTH = 1280;`
- Class constants: (static constexpr)

**Special Prefixes:**
- Private/protected members: `m_` prefix → `m_Window`, `m_GraphicsContext`, `m_Running`

**Important Note:** The .clang-tidy configuration treats warnings as errors (`WarningsAsErrors: "*"`), so naming violations will fail the build.

### Code Style (clang-format)

- **Indentation**: 2 spaces (never tabs)
- **Column limit**: 80 characters (strictly enforced)
- **Brace style**: Custom Allman-style
  - Classes, structs, functions, enums: opening brace on new line
  - Control statements: multiline break
- **Pointer alignment**: Left (`int* ptr`, not `int *ptr`)
- **Include order**: Standard library → third-party headers → project headers
- **Line endings**: LF (Unix-style)
- **Trailing return types**: Required for all functions

### Modern C++23 Practices

```cpp
// ✓ Use trailing return types (required)
auto GetWidth() const -> uint32_t;
auto Create(const WindowProps& props) -> std::unique_ptr<Window>;

// ✓ Use [[nodiscard]] for getters and important return values
[[nodiscard]] auto GetWidth() const -> uint32_t;
[[nodiscard]] auto IsVSync() const -> bool;

// ✓ Delete unwanted special members explicitly
Window(const Window&) = delete;
Window(Window&&) = delete;
auto operator=(const Window&) -> Window& = delete;
auto operator=(Window&&) -> Window& = delete;

// ✓ Use virtual destructors with = default or override
virtual ~Window() = default;
~LinuxWindow() override;

// ✓ Use explicit for constructors that can be called with one argument
explicit WindowProps(std::string title = "Lumina Engine", ...);

// ✓ Use constexpr for compile-time constants
constexpr uint32_t WIDTH = 1280;
constexpr int MAJOR_VER = 4;

// ✓ Prefer std::unique_ptr for ownership
std::unique_ptr<Window> m_Window;
std::unique_ptr<GraphicsContext> m_GraphicsContext;

// ✓ Use uniform initialization with braces
uint32_t Width {WIDTH};
GLFWwindow* m_Window {nullptr};

// ✓ Use [[maybe_unused]] to suppress warnings
void OnEvent([[maybe_unused]] void* event);

// ✓ Use enum class for type-safe enums
enum class ShaderType : std::uint8_t { Vertex, Fragment, Compute };

// ✓ Use static file-scope functions instead of anonymous namespaces
static void GLFWErrorCallback(int error, const char* description) { ... }
```

## Dependencies (vcpkg.json)

### Required Dependencies
- **glad** (≥0.1.36): OpenGL function loader
  - Features: `gl-api-46` (OpenGL 4.6)
- **glfw3** (≥3.4): Cross-platform windowing and input
- **spdlog** (≥1.15.0): Fast C++ logging library
  - Used in header-only mode with `SPDLOG_USE_STD_FORMAT`
- **shader-slang** (≥2024.15.2): High-performance shader compiler

### Test Dependencies (optional feature)
- **catch2** (≥3.7.1): Unit testing framework

### Dependency Management
```bash
# vcpkg will automatically install dependencies when configuring CMake
# No manual installation required if using CMake presets

# To install with test dependencies:
# Configure CMake with test feature enabled in vcpkg
```

## Build System

### CMake Configuration
- Minimum version: 3.14
- C++ standard: C++23 (required, no extensions)
- Library type: Static (shared library support available but not default)
- Export headers: Auto-generated with `GenerateExportHeader`
- Target name: `lumina::lumina`

### Important CMake Definitions
- `SPDLOG_USE_STD_FORMAT`: Use C++20 std::format instead of fmt
- `GLFW_INCLUDE_NONE`: Don't include OpenGL headers from GLFW (using GLAD)
- `lumina_DEVELOPER_MODE`: Enable developer targets and stricter checks

### Build Workflow
```bash
# Using CMake Presets (recommended)
cmake --preset=dev           # Configure with developer mode
cmake --build --preset=dev   # Build
ctest --preset=dev           # Run tests

# Using Taskfile (convenience wrapper)
task build                   # Configure + build
task run                     # Build + run sandbox
task test                    # Run tests
task format                  # Format code with clang-format
task format-check            # Check formatting (CI)
task tidy                    # Run clang-tidy static analysis
task tidy-fix                # Auto-fix clang-tidy issues
task clean                   # Remove build directory
```

### Build Output Structure
```
build/dev/
├── example/
│   └── sandbox              # Example application
├── CMakeCache.txt
└── ... (other build artifacts)
```

## Core Systems

### 1. Logger System (Core/Logger.hpp)

Wrapper around spdlog providing static logging interface:

```cpp
// Initialize once at application start
Logger::Init();

// Logging methods (with std::format syntax)
Logger::Trace("Detailed debug info: {}", value);
Logger::Info("General information: {}", message);
Logger::Warn("Warning message: {}", issue);
Logger::Error("Error occurred: {}", error_code);
Logger::Critical("Critical failure: {}", fatal_error);

// Access underlying spdlog instance if needed
auto logger = Logger::GetLogger();
```

**Features:**
- Colored console output
- Thread-safe
- Uses C++20 `std::format` syntax (via `SPDLOG_USE_STD_FORMAT`)
- Static interface for convenient global access

### 2. Application Framework (Core/Application.hpp)

Central application class managing the engine lifecycle:

```cpp
class Application {
public:
    Application();              // Creates window, sets up event callbacks
    void Run();                 // Main loop (polls events, updates, renders)
    void OnEvent(void* event);  // Event handler callback

private:
    std::unique_ptr<Window> m_Window;
    bool m_Running = true;
    float m_LastFrameTime = 0.0F;
};
```

**Usage Pattern:**
1. Derive from `Application` in client code
2. Implement `CreateApplication()` factory function
3. Call `Logger::Init()` and `app->Run()` in main

### 3. Window System (Core/Window.hpp)

Platform-abstracted window interface:

```cpp
struct WindowProps {
    std::string Title;
    WindowDimensions Dimensions;  // Width, Height
    bool VSync;
    std::function<void(void*)> EventCallback;
};

class Window {
public:
    virtual void Init(WindowProps props) = 0;
    virtual void OnUpdate() = 0;  // Poll events, swap buffers

    [[nodiscard]] virtual auto GetWidth() const -> uint32_t = 0;
    [[nodiscard]] virtual auto GetHeight() const -> uint32_t = 0;
    [[nodiscard]] virtual auto GetNativeWindow() const -> void* = 0;

    virtual void SetVSync(bool enabled) = 0;
    [[nodiscard]] virtual auto IsVSync() const -> bool = 0;

    static auto Create(const WindowProps& props) -> std::unique_ptr<Window>;
};
```

### 4. Graphics Context (Renderer/GraphicsContext.hpp)

Graphics API abstraction layer:

```cpp
class GraphicsContext {
public:
    virtual void SetGLFWWindow(struct GLFWwindow* window) = 0;
    virtual void Init() = 0;        // Initialize graphics API
    virtual void LoadFns() = 0;     // Load function pointers
    virtual void SwapBuffers() = 0; // Present frame
};

// Current implementation: OpenGLContext
// - Initializes GLFW with OpenGL 4.6 Core Profile
// - Loads OpenGL functions via GLAD
// - Sets up error callbacks
```

### 5. Shader Compiler (Renderer/ShaderCompiler.hpp)

Shader compilation system using Slang:

```cpp
enum class ShaderType : std::uint8_t {
    Vertex,
    Fragment,
    Compute
};

using ShaderSources = std::map<ShaderType, std::vector<uint32_t>>;

class ShaderCompiler {
    static auto Compile(const std::string& shader_path) -> ShaderSources;
};
```

**Note:** Implementation in progress. Will compile Slang shaders to SPIR-V.

## Static Analysis and Code Quality

### clang-tidy Configuration

**Strictness Level:** Maximum (warnings as errors)
- `WarningsAsErrors: "*"` - ALL warnings treated as errors
- Nearly all checks enabled
- Pedantic mode for narrowing conversions

**Disabled Checks:**
- `google-readability-todo`: Allow TODO comments
- `altera-*`, `fuchsia-*` (except multiple inheritance), `llvmlibc-*`: Not relevant
- `bugprone-easily-swappable-parameters`: Too noisy
- `modernize-use-nodiscard`: Too aggressive
- `misc-include-cleaner`, `misc-use-anonymous-namespace`: Style preferences

**How to Run:**
```bash
task tidy         # Check for issues
task tidy-fix     # Auto-fix where possible
```

### clang-format Configuration

- Comprehensive formatting rules (see `.clang-format`)
- 80-column limit strictly enforced
- Custom Allman-style braces
- No alignment of consecutive declarations/assignments
- Always break before template declarations

**How to Run:**
```bash
task format       # Format all source files
task format-check # Check formatting (CI)
```

### codespell Configuration

- Spell checking for comments, strings, and documentation
- Skips: `.git/`, `build/`, `prefix/`, `conan/`
- Multiple dictionaries enabled

## Compiler Requirements

### Warning Flags (GCC/Clang)
Comprehensive warnings enabled (see CMakePresets.json):
- `-Wall -Wextra -Wpedantic`
- `-Wconversion -Wsign-conversion`
- `-Wcast-qual -Wcast-align`
- `-Wshadow -Wundef -Werror=float-equal`
- `-Wnull-dereference -Wdouble-promotion`
- `-Wimplicit-fallthrough`
- `-Woverloaded-virtual -Wnon-virtual-dtor -Wold-style-cast`

### Security Hardening
- Stack protection: `-fstack-protector-strong`
- Control flow protection: `-fcf-protection=full`
- Stack clash protection: `-fstack-clash-protection`
- Fortification: `-D_FORTIFY_SOURCE=3`
- Linker flags: `-z,noexecstack -z,relro -z,now`

## Important Patterns and Anti-Patterns

### ✓ DO

**Architecture:**
- Use platform abstraction for OS-specific code
- Place public API in `include/`, implementation in `source/`
- Keep platform-specific code in `Platform/` subdirectories
- Use abstract interfaces with virtual destructors for polymorphism

**Naming:**
- Use `CamelCase` for classes, methods, public members
- Use `snake_case` for local variables and parameters
- Use `m_` prefix for ALL private/protected members
- Use `UPPER_CASE` for constants and macros

**Modern C++:**
- Use trailing return types: `auto Func() -> ReturnType`
- Delete copy/move operations for non-copyable types
- Use `[[nodiscard]]` for getters and important returns
- Use `[[maybe_unused]]` to suppress unused parameter warnings
- Use `explicit` for single-argument constructors
- Use `virtual` destructors with `= default` or `override`
- Use `constexpr` for compile-time constants
- Use `std::unique_ptr` for ownership transfer
- Use uniform initialization `{}` for member initialization
- Use `enum class` for type-safe enumerations

**Code Quality:**
- Keep lines under 80 characters (hard limit)
- Run `task format` before committing
- Run `task tidy` to catch errors early
- Use `Logger` for all diagnostic output (not std::cout)
- Handle errors with exceptions (not error codes)
- Use `const` liberally

### ✗ DON'T

**Architecture:**
- Don't put platform-specific code in `Core/`
- Don't expose implementation details in public headers
- Don't use raw pointers for ownership

**Naming:**
- Don't use snake_case for class/method names (violates clang-tidy)
- Don't forget `m_` prefix on private/protected members (build will fail)
- Don't use Hungarian notation (except m_ prefix)

**Code Style:**
- Don't use tabs for indentation (use 2 spaces)
- Don't exceed 80 columns (hard limit, will fail CI)
- Don't use C-style casts (use `static_cast`, `reinterpret_cast`, etc.)
- Don't use old-style function declarations
- Don't ignore compiler warnings (build will fail)
- Don't commit code that fails `clang-tidy` (CI will fail)
- Don't use `printf`/`std::cout` for logging (use `Logger`)

**Modern C++:**
- Don't use raw `new`/`delete` (use smart pointers)
- Don't forget `override` keyword on virtual function overrides
- Don't use `NULL` or `0` for null pointers (use `nullptr`)
- Don't forget trailing return types (style requirement)

## Testing Strategy

- **Framework**: Catch2 v3
- **Location**: `test/source/`
- **Style**: BDD-style test cases with `TEST_CASE` and `REQUIRE`
- **Execution**: `task test` or `ctest --preset=dev`

Example test:
```cpp
#include <catch2/catch_test_macros.hpp>
#include "Core/Application.hpp"

TEST_CASE("Application creates window", "[application]") {
    Logger::Init();
    auto app = std::make_unique<Application>();
    REQUIRE(app != nullptr);
}
```

## Common Tasks for AI Assistance

When helping with this project, please:

1. **Follow naming strictly**: CamelCase for classes/methods, snake_case for locals/params, m_ for private members
2. **Use trailing return types**: Always use `auto Func() -> ReturnType` syntax
3. **Keep 80-column limit**: This is strictly enforced by clang-format
4. **Maintain platform abstraction**: Keep platform code in `Platform/` subdirectories
5. **Use the Logger**: Never use `std::cout`, always use `Logger::Info()`, etc.
6. **Add [[nodiscard]]**: For getters and functions returning important values
7. **Use constexpr**: For compile-time constants instead of const
8. **Check clang-tidy**: Remember warnings are errors, code must be clean
9. **Headers in include/**: Public API goes in `include/`, implementation in `source/`
10. **Match directory structure**: `include/` and `source/` should mirror each other

## Current Development Status

### Implemented
- ✓ Core application framework with main loop
- ✓ Logger system (spdlog wrapper)
- ✓ Window abstraction with Linux/GLFW implementation
- ✓ OpenGL context management (4.6 Core Profile)
- ✓ Graphics API abstraction layer
- ✓ vcpkg dependency management
- ✓ Comprehensive build system with presets
- ✓ Static analysis and formatting infrastructure

### In Progress
- ⚙ Shader compiler integration (Slang → SPIR-V)
- ⚙ Windows window implementation
- ⚙ Event system (currently placeholder)

### Planned
- ⏳ Input handling (keyboard, mouse, gamepad)
- ⏳ Renderer architecture (renderer, render commands, etc.)
- ⏳ Entity-Component-System (ECS)
- ⏳ Scene management
- ⏳ Asset management pipeline
- ⏳ Physics integration
- ⏳ Audio system
- ⏳ Scripting support

## Build Troubleshooting

### Common Issues

**vcpkg dependencies not found:**
```bash
# Ensure vcpkg is integrated with CMake
# CMake should automatically fetch dependencies via manifest mode
# Check that VCPKG_ROOT or CMAKE_TOOLCHAIN_FILE is set
```

**clang-tidy errors:**
```bash
# Run auto-fix first
task tidy-fix

# Check remaining errors
task tidy

# Common issues:
# - Missing m_ prefix on private members
# - Wrong naming convention (use CamelCase for methods)
# - Missing [[nodiscard]] on important return values
```

**Format check failures:**
```bash
# Auto-format all files
task format

# Verify formatting
task format-check
```

## References and Resources

- [CMake Documentation](https://cmake.org/documentation/)
- [vcpkg Documentation](https://vcpkg.io/)
- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines)
- [Catch2 Documentation](https://github.com/catchorg/Catch2)
- [spdlog Documentation](https://github.com/gabime/spdlog)
- [GLFW Documentation](https://www.glfw.org/documentation.html)
- [Slang Documentation](https://shader-slang.com/)

## Notes for AI Assistants

- **This is a learning/portfolio project**: Code quality and best practices are heavily emphasized
- **Warnings as errors**: The build will fail on ANY warning, including clang-tidy checks
- **Strict naming enforcement**: Use the exact naming conventions or the build fails
- **80-column hard limit**: Lines over 80 characters will fail CI
- **Prefer simplicity**: Don't over-engineer solutions
- **Platform support**: Primarily Linux, Windows support in development
- **Graphics focus**: The engine's primary goal is graphics rendering with OpenGL 4.6
- **Early stage**: Many systems are stubs or not yet implemented
- **Documentation matters**: Keep code well-documented for learning purposes
- **Use Logger everywhere**: Replace any std::cout/printf with Logger::Info/Debug/etc.
- **Trailing return types required**: All functions must use `auto name() -> type` syntax
