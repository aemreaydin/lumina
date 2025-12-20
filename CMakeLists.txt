cmake_minimum_required(VERSION 3.14)

include(cmake/prelude.cmake)

project(
    lumina
    VERSION 0.1.0
    DESCRIPTION "A Game Engine with a focus on Graphics"
    HOMEPAGE_URL "https://example.com/"
    LANGUAGES CXX
)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

# ---- Declare library ----

add_library(
    lumina_lumina
    source/Core/Application.cpp
    source/Core/ConfigLoader.cpp
    source/Core/Input.cpp
    source/Core/Logger.cpp
    source/Core/Window.cpp
    # Platform
    source/Platform/Linux/LinuxWindow.cpp
    # source/Platform/Windows/WindowsWindow.cpp
    # Renderer
    source/Renderer/Camera.cpp
    source/Renderer/CameraController.cpp
    source/Renderer/ShaderCompiler.cpp
    # Model
    source/Renderer/Model/Vertex.cpp
    source/Renderer/Model/Material.cpp
    source/Renderer/Model/Mesh.cpp
    source/Renderer/Model/Model.cpp
    source/Renderer/Model/ModelLoader.cpp
    # Asset
    source/Renderer/Asset/AssetManager.cpp
    # Scene
    source/Renderer/Scene/Transform.cpp
    source/Renderer/Scene/SceneNode.cpp
    source/Renderer/Scene/Scene.cpp
    source/Renderer/Scene/SceneRenderer.cpp
    # RHI
    source/Renderer/RHI/RHIDevice.cpp
    source/Renderer/RHI/OpenGL/OpenGLDevice.cpp
    source/Renderer/RHI/OpenGL/OpenGLSwapchain.cpp
    source/Renderer/RHI/OpenGL/OpenGLCommandBuffer.cpp
    source/Renderer/RHI/Vulkan/VulkanDevice.cpp
    source/Renderer/RHI/Vulkan/VulkanSwapchain.cpp
    source/Renderer/RHI/Vulkan/VulkanCommandBuffer.cpp
    source/Renderer/RHI/Vulkan/VulkanBuffer.cpp
    source/Renderer/RHI/Vulkan/VulkanShaderModule.cpp
    source/Renderer/RHI/Vulkan/VulkanDescriptorSet.cpp
    source/Renderer/RHI/Vulkan/VulkanPipelineLayout.cpp
    source/Renderer/RHI/Vulkan/VulkanTexture.cpp
    source/Renderer/RHI/Vulkan/VulkanSampler.cpp
    source/Renderer/RHI/OpenGL/OpenGLBuffer.cpp
    source/Renderer/RHI/OpenGL/OpenGLShaderModule.cpp
    source/Renderer/RHI/OpenGL/OpenGLDescriptorSet.cpp
    source/Renderer/RHI/OpenGL/OpenGLPipelineLayout.cpp
    source/Renderer/RHI/OpenGL/OpenGLTexture.cpp
    source/Renderer/RHI/OpenGL/OpenGLSampler.cpp
)
add_library(lumina::lumina ALIAS lumina_lumina)

include(GenerateExportHeader)
generate_export_header(
    lumina_lumina
    BASE_NAME lumina
    EXPORT_FILE_NAME export/lumina/lumina_export.hpp
    CUSTOM_CONTENT_FROM_VARIABLE pragma_suppress_c4251
)

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(lumina_lumina PUBLIC LUMINA_STATIC_DEFINE)
endif()

set_target_properties(
    lumina_lumina
    PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN YES
        VERSION "${PROJECT_VERSION}"
        SOVERSION "${PROJECT_VERSION_MAJOR}"
        EXPORT_NAME lumina
        OUTPUT_NAME lumina
)

target_include_directories(
    lumina_lumina
    ${warning_guard}
    PUBLIC "\$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
)

target_include_directories(
    lumina_lumina
    SYSTEM
    PUBLIC "\$<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/export>"
)

target_compile_features(lumina_lumina PUBLIC cxx_std_23)

find_package(spdlog CONFIG REQUIRED)
target_link_libraries(lumina_lumina PUBLIC spdlog::spdlog_header_only)

find_package(glad CONFIG REQUIRED)
target_link_libraries(lumina_lumina PUBLIC glad::glad)

find_package(SDL3 CONFIG REQUIRED)
target_link_libraries(lumina_lumina PUBLIC SDL3::SDL3)

find_package(volk CONFIG REQUIRED)
target_link_libraries(lumina_lumina PUBLIC volk::volk volk::volk_headers)

find_package(unofficial-vulkan-memory-allocator-hpp CONFIG REQUIRED)
target_link_libraries(
    lumina_lumina
    PUBLIC unofficial::VulkanMemoryAllocator-Hpp::VulkanMemoryAllocator-Hpp
)

find_package(tomlplusplus CONFIG REQUIRED)

target_link_libraries(lumina_lumina PUBLIC tomlplusplus::tomlplusplus)

find_package(glm CONFIG REQUIRED)
target_link_libraries(lumina_lumina PUBLIC glm::glm)

find_package(tinyobjloader CONFIG REQUIRED)
target_link_libraries(lumina_lumina PUBLIC tinyobjloader::tinyobjloader)

find_library(SLANG_LIBRARY slang REQUIRED)
target_link_libraries(lumina_lumina PUBLIC ${SLANG_LIBRARY})

target_compile_definitions(
    lumina_lumina
    PUBLIC SPDLOG_USE_STD_FORMAT
)

# ---- Install rules ----

if(NOT CMAKE_SKIP_INSTALL_RULES)
    include(cmake/install-rules.cmake)
endif()

# ---- Examples ----

if(PROJECT_IS_TOP_LEVEL)
    option(BUILD_EXAMPLES "Build examples tree." "${lumina_DEVELOPER_MODE}")
    if(BUILD_EXAMPLES)
        add_subdirectory(example)
    endif()
endif()

# ---- Developer mode ----

if(NOT lumina_DEVELOPER_MODE)
    return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
    message(
        AUTHOR_WARNING
        "Developer mode is intended for developers of lumina"
    )
endif()

include(cmake/dev-mode.cmake)
