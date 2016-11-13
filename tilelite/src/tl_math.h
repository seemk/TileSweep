#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

const double PI = 3.14159265358979323846;

typedef struct {
  double x;
  double y;
} vec2d;

typedef struct {
  int32_t x;
  int32_t y;
} vec2i;

typedef struct {
  double x;
  double y;
  double z;
} vec3d;

vec2d xyz_to_latlon(vec3d xyz);
vec2i latlon_to_xyz(vec2d coord, int zoom);

void latlon_to_xyz_multi(const vec2d* coordinates, int len, int zoom,
                         vec2i* out);

#ifdef __cplusplus
}
#endif
