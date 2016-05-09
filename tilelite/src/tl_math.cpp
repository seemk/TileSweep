#include "tl_math.h"
#include <cmath>

vec2d xyz_to_latlon(vec3d xyz) {
  vec2d res;

  double n = std::pow(2.0, xyz.z);
  res.x = xyz.x / n * 360.0 - 180.0;
  res.y =
      std::atan(std::sinh(PI * (1.0 - 2.0 * xyz.y / n))) * 180.0 / PI;
  return res;
}

vec2i latlon_to_xyz(vec2d coord, int zoom) {
  double lat_rad = deg_to_rad(coord.y);
  double n = std::pow(2.0, zoom);

  vec2i res;
  res.x = int((coord.x + 180.0) / 360.0 * n);
  res.y = int((1.0 - std::log(std::tan(lat_rad) + (1.0 / std::cos(lat_rad))) / PI) / 2.0 * n);

  return res;
}

void latlon_to_xyz(const vec2d* coordinates, int len, int zoom, vec2i* out) {
  for (int i = 0; i < len; i++) {
    vec2d coord = coordinates[i];
    out[i] = latlon_to_xyz(coord, zoom);
  }
}
