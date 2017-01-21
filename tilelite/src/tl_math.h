#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PI 3.14159265358979323846

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

typedef struct {
  vec2i top_left;
  vec2i bot_right;
} bounding_boxi;

typedef struct {
  vec2d top_left;
  vec2d bot_right;
} bounding_boxd;

static inline double sign(double x) {
  if (x > 0.0) return 1.0;
  if (x < 0.0) return -1.0;

  return 0.0;
}

vec2d mercator_to_tile(double x, double y, int32_t zoom, int32_t tile_size);
bounding_boxd tile_to_mercator(int32_t x, int32_t y, int32_t z,
                               int32_t tile_size);

#ifdef __cplusplus
}
#endif
