# Lumina

A modular graphics engine with Vulkan and OpenGL backends.

## Features

- **Dual-API rendering** - Vulkan and OpenGL 4.6 with runtime selection
- **RHI abstraction** - Cross-API rendering hardware interface
- **Hierarchical scene graph** - Parent-child node relationships with transforms
- **Model loading** - OBJ format support via tinyobjloader
- **Shader compilation** - Slang compiler for cross-API shaders
- **Camera systems** - Orbit and FPS camera controllers
- **Asset management** - Centralized resource loading and caching

## Building

### Prerequisites

- C++23 compiler (GCC 13+, Clang 17+, MSVC 2022+)
- CMake 3.14+
- vcpkg
- [Task](https://taskfile.dev/) (optional, for convenience)

### Quick Start

```sh
task configure
task build
task run
```

Or manually:

```sh
cmake --preset default
cmake --build build/dev
./build/dev/example/scene_demo
```

## Project Structure

```
lumina/
├── source/              # Engine implementation
│   ├── Core/            # Application framework, window, input
│   ├── Platform/        # Platform-specific code (Linux, Windows, Mac)
│   └── Renderer/
│       ├── RHI/         # Rendering hardware interface
│       │   ├── OpenGL/  # OpenGL backend
│       │   └── Vulkan/  # Vulkan backend
│       ├── Asset/       # Asset management
│       ├── Model/       # Model/mesh loading
│       └── Scene/       # Scene graph
├── include/             # Public headers
├── example/             # Example applications
├── shaders/             # Core shaders (.slang)
└── cmake/               # CMake utilities
```

## Architecture

The engine uses a **Rendering Hardware Interface (RHI)** abstraction layer:

```
Application
    │
    ▼
┌─────────────────┐
│   RHI Layer     │  ← Abstract interfaces (RHIDevice, RHICommandBuffer, etc.)
└─────────────────┘
    │         │
    ▼         ▼
┌───────┐ ┌────────┐
│Vulkan │ │ OpenGL │  ← Concrete implementations
└───────┘ └────────┘
```

Backend selection is configured in `config.toml`:

```toml
[renderer]
api = "vulkan"  # or "opengl"
```

## Dependencies

| Library | Purpose | Version |
|---------|---------|---------|
| SDL3 | Window/input | 3.2.28+ |
| Vulkan (volk) | Graphics API | - |
| glad | OpenGL loader | 4.6 |
| Slang | Shader compiler | - |
| GLM | Math library | 1.0.1+ |
| spdlog | Logging | 1.15+ |
| toml++ | Config parsing | 3.4+ |
| tinyobjloader | OBJ loading | 2.0.0rc13+ |
| stb | Image loading | 2024-07-29 |

## Examples

| Example | Description |
|---------|-------------|
| `triangle` | Basic triangle rendering |
| `texture` | Textured mesh with samplers |
| `depth` | Depth testing and 3D rendering |
| `scene_demo` | Full scene graph with model loading |

Run examples:

```sh
task run              # Runs scene_demo (default)
task run -- triangle  # Runs specific example
```

## Configuration

`config.toml` options:

```toml
[logger]
level = "trace"  # trace, debug, info, warn, error

[renderer]
api = "vulkan"          # vulkan, opengl
validation = true       # Enable validation layers (Vulkan)
```

## Contributing

### Code Style

The project uses clang-format and clang-tidy. Run before committing:

```sh
task format  # Format code
task tidy    # Static analysis
```

### Available Tasks

```sh
task configure  # CMake configuration
task build      # Build engine and examples
task rebuild    # Clean rebuild
task assets     # Copy assets to build directory
task clean      # Remove build artifacts
```

## Roadmap

- [ ] PBR materials
- [ ] Shadow mapping
- [ ] glTF model support
- [ ] Compute shader support
- [ ] Multi-threaded rendering
