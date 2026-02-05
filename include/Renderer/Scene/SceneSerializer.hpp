#ifndef RENDERER_SCENE_SCENESERIALIZER_HPP
#define RENDERER_SCENE_SCENESERIALIZER_HPP

#include <optional>
#include <string>

#include <linalg/vec.hpp>

#include "Renderer/Scene/Scene.hpp"

class Camera;
class AssetManager;

struct SceneCameraData
{
  linalg::Vec3 Position {0.0F, 0.0F, 5.0F};
  linalg::Vec3 Target {0.0F, 0.0F, 0.0F};
  float FOV {45.0F};
  float NearPlane {0.01F};
  float FarPlane {1000.0F};
};

struct SceneLoadResult
{
  Scene SceneData;
  std::optional<SceneCameraData> Camera;
};

class SceneSerializer
{
public:
  static auto Load(const std::string& path, AssetManager& assets)
      -> SceneLoadResult;
  static void Save(const Scene& scene, const Camera& camera,
                   const std::string& path);
};

#endif
