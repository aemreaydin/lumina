#ifndef RENDERER_SCENE_LIGHTDATA_HPP
#define RENDERER_SCENE_LIGHTDATA_HPP

#include <linalg/vec.hpp>

struct PointLightData
{
  linalg::Vec3 Position;
  float Radius;
  linalg::Vec3 Color;
  float Intensity;
};

struct DirectionalLightData
{
  linalg::Vec3 Direction;
  float Intensity;
  linalg::Vec3 Color;
  float _pad {0.0F};
};

#endif
