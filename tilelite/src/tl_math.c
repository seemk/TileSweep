#include "tl_math.h"
#include <assert.h>
#include <math.h>

#define EARTH_RADIUS 6378137.0
#define MERCATOR_SHIFT_ORIGIN 20037508.342789244

static inline double resolution(double zoom, double tile_size) {
  return (2.0 * PI * EARTH_RADIUS) / (tile_size * exp2(zoom));
}

static inline vec2d pixel_to_meter(double x, double y, double zoom,
                                   double tile_size) {
  const double reso = resolution(zoom, tile_size);
  return (vec2d){.x = x * reso - MERCATOR_SHIFT_ORIGIN,
                 .y = y * reso - MERCATOR_SHIFT_ORIGIN};
}

vec2d mercator_to_tile(double x, double y, int32_t zoom, int32_t tile_size) {
  assert(tile_size > 0 && zoom >= 0);
  const double size = (double)tile_size;
  const double reso = resolution(zoom, size);

  const double px = (x + MERCATOR_SHIFT_ORIGIN) / reso;
  const double py = (-y + MERCATOR_SHIFT_ORIGIN) / reso;

  return (vec2d){.x = ceil(px / size) - 1.0, .y = ceil(py / size) - 1.0};
}

bounding_boxd tile_to_mercator(int32_t x, int32_t y, int32_t z,
                               int32_t tile_size) {
  const double zoom = (double)z;
  const double size = (double)tile_size;
  const double tx = (double)x;
  const double ty = (double)((1 << z) - y - 1);
  const vec2d top_left = pixel_to_meter(tx * size, ty * size, zoom, size);
  const vec2d bot_right =
      pixel_to_meter((tx + 1.0) * size, (ty + 1.0) * size, zoom, size);
  return (bounding_boxd){.top_left = top_left, .bot_right = bot_right};
}
