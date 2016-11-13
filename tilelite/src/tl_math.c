#include "tl_math.h"
#include <math.h>

vec2d xyz_to_latlon(vec3d xyz) {
  vec2d res;

  double n = pow(2.0, xyz.z);
  res.x = xyz.x / n * 360.0 - 180.0;
  res.y = atan(sinh(PI * (1.0 - 2.0 * xyz.y / n))) * 180.0 / PI;
  return res;
}

vec2i latlon_to_xyz(vec2d coord, int zoom) {
  double lat_rad = coord.y * PI / 180.0;
  double n = pow(2.0, zoom);

  vec2i res;
  res.x = (int32_t)((coord.x + 180.0) / 360.0 * n);
  res.y = (int32_t)(
      ((1.0 - log(tan(lat_rad) + (1.0 / cos(lat_rad))) / PI) / 2.0 * n));

  return res;
}

void latlon_to_xyz_multi(const vec2d* coordinates, int len, int zoom,
                         vec2i* out) {
  for (int i = 0; i < len; i++) {
    vec2d coord = coordinates[i];
    out[i] = latlon_to_xyz(coord, zoom);
  }
}
