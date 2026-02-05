#include "Renderer/Scene/SceneSerializer.hpp"

#include <fstream>

#include <nlohmann/json.hpp>

#include "Core/Logger.hpp"
#include "Renderer/Asset/AssetManager.hpp"
#include "Renderer/Camera.hpp"
#include "Renderer/Scene/Scene.hpp"
#include "Renderer/Scene/SceneNode.hpp"

using json = nlohmann::json;

static auto Vec3ToJson(const linalg::Vec3& v) -> json
{
  return json::array({v.x(), v.y(), v.z()});
}

static auto JsonToVec3(const json& j) -> linalg::Vec3
{
  return linalg::Vec3 {j[0].get<float>(), j[1].get<float>(),
                       j[2].get<float>()};
}

static auto SerializeLight(const LightComponent& lc) -> json
{
  json light;
  light["type"] =
      (lc.LightType == LightComponent::Type::Directional) ? "directional"
                                                           : "point";
  light["color"] = Vec3ToJson(lc.Color);
  light["intensity"] = lc.Intensity;

  if (lc.LightType == LightComponent::Type::Point) {
    light["radius"] = lc.Radius;
  } else {
    light["direction"] = Vec3ToJson(lc.Direction);
  }

  return light;
}

static auto DeserializeLight(const json& j) -> LightComponent
{
  LightComponent lc;
  std::string type = j.value("type", "point");
  lc.LightType = (type == "directional") ? LightComponent::Type::Directional
                                         : LightComponent::Type::Point;
  if (j.contains("color")) {
    lc.Color = JsonToVec3(j["color"]);
  }
  lc.Intensity = j.value("intensity", 1.0F);

  if (lc.LightType == LightComponent::Type::Point) {
    lc.Radius = j.value("radius", 10.0F);
  } else {
    if (j.contains("direction")) {
      lc.Direction = JsonToVec3(j["direction"]);
    }
  }

  return lc;
}

static auto SerializeNode(const SceneNode& node) -> json
{
  json j;
  j["name"] = node.GetName();

  const auto& pos = node.GetPosition();
  const auto euler = node.GetTransform().GetRotationEuler();
  const auto& scale = node.GetTransform().GetScale();

  // Only write non-default transform values
  if (pos.x() != 0.0F || pos.y() != 0.0F || pos.z() != 0.0F) {
    j["position"] = Vec3ToJson(pos);
  }
  if (euler.x() != 0.0F || euler.y() != 0.0F || euler.z() != 0.0F) {
    j["rotation"] = Vec3ToJson(euler);
  }
  if (scale.x() != 1.0F || scale.y() != 1.0F || scale.z() != 1.0F) {
    j["scale"] = Vec3ToJson(scale);
  }

  if (!node.IsVisible()) {
    j["visible"] = false;
  }

  if (!node.GetModelPath().empty()) {
    j["model"] = node.GetModelPath();
  }

  if (node.HasLight()) {
    j["light"] = SerializeLight(*node.GetLight());
  }

  if (node.GetChildCount() > 0) {
    json children = json::array();
    for (const auto& child : node.GetChildren()) {
      children.push_back(SerializeNode(*child));
    }
    j["children"] = children;
  }

  return j;
}

static void DeserializeNode(const json& j, SceneNode* node, Scene& scene,
                            AssetManager& assets)
{
  if (j.contains("position")) {
    node->SetPosition(JsonToVec3(j["position"]));
  }
  if (j.contains("rotation")) {
    node->SetRotationEuler(JsonToVec3(j["rotation"]));
  }
  if (j.contains("scale")) {
    auto scale_val = j["scale"];
    if (scale_val.is_number()) {
      node->SetScale(scale_val.get<float>());
    } else {
      node->SetScale(JsonToVec3(scale_val));
    }
  }

  if (j.contains("visible")) {
    node->SetVisible(j["visible"].get<bool>());
  }

  if (j.contains("model")) {
    std::string model_path = j["model"].get<std::string>();
    node->SetModelPath(model_path);
    auto model = assets.LoadModel(model_path);
    if (model) {
      node->SetModel(model);
    } else {
      Logger::Warn("SceneSerializer: Failed to load model '{}'", model_path);
    }
  }

  if (j.contains("light")) {
    node->SetLight(DeserializeLight(j["light"]));
  }

  if (j.contains("children")) {
    for (const auto& child_json : j["children"]) {
      std::string name = child_json.value("name", "Node");
      auto* child = scene.CreateNode(name, node);
      DeserializeNode(child_json, child, scene, assets);
    }
  }
}

auto SceneSerializer::Load(const std::string& path, AssetManager& assets)
    -> SceneLoadResult
{
  std::ifstream file(path);
  if (!file.is_open()) {
    throw std::runtime_error("Failed to open scene file: " + path);
  }

  json data = json::parse(file);
  const auto& scene_data = data["scene"];

  std::string scene_name = scene_data.value("name", "Scene");
  SceneLoadResult result;
  result.SceneData = Scene(scene_name);

  // Load camera settings
  if (scene_data.contains("camera")) {
    const auto& cam = scene_data["camera"];
    SceneCameraData cam_data;
    if (cam.contains("position")) {
      cam_data.Position = JsonToVec3(cam["position"]);
    }
    if (cam.contains("target")) {
      cam_data.Target = JsonToVec3(cam["target"]);
    }
    cam_data.FOV = cam.value("fov", 45.0F);
    cam_data.NearPlane = cam.value("nearPlane", 0.01F);
    cam_data.FarPlane = cam.value("farPlane", 1000.0F);
    result.Camera = cam_data;
  }

  // Load nodes
  if (scene_data.contains("nodes")) {
    for (const auto& node_json : scene_data["nodes"]) {
      std::string name = node_json.value("name", "Node");
      SceneNode* scene_node = result.SceneData.CreateNode(name);
      DeserializeNode(node_json, scene_node, result.SceneData, assets);
    }
  }

  Logger::Info("SceneSerializer: Loaded scene '{}' from '{}'", scene_name,
               path);
  return result;
}

static auto SerializeCamera(const Camera& camera) -> json
{
  json cam;
  cam["position"] = Vec3ToJson(camera.GetPosition());
  cam["fov"] = camera.GetFOV();
  cam["nearPlane"] = camera.GetNearPlane();
  cam["farPlane"] = camera.GetFarPlane();

  // Compute target from position + forward direction
  auto target = camera.GetPosition() + camera.GetForward() * 10.0F;
  cam["target"] = Vec3ToJson(target);

  return cam;
}

void SceneSerializer::Save(const Scene& scene, const Camera& camera,
                           const std::string& path)
{
  json data;
  json scene_data;

  scene_data["name"] = scene.GetName();
  scene_data["camera"] = SerializeCamera(camera);

  // Serialize root's children as top-level nodes
  json nodes = json::array();
  const auto& root = scene.GetRoot();
  for (const auto& child : root.GetChildren()) {
    nodes.push_back(SerializeNode(*child));
  }
  scene_data["nodes"] = nodes;

  data["scene"] = scene_data;

  std::ofstream file(path);
  if (!file.is_open()) {
    throw std::runtime_error("Failed to write scene file: " + path);
  }

  file << data.dump(2);
  Logger::Info("SceneSerializer: Saved scene '{}' to '{}'", scene.GetName(),
               path);
}
