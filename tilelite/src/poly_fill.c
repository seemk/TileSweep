#include "poly_fill.h"
#include <float.h>
#include <math.h>
#include "stretchy_buffer.h"

static inline double fraction(double x) { return x - floor(x); }

vec2d* raycast(vec2d begin, vec2d end) {
  const double dist_x = end.x - begin.x;
  const double dist_y = end.y - begin.y;

  const double step_x = dist_x < 0 ? -1.0 : 1.0;
  const double step_y = dist_y < 0 ? -1.0 : 1.0;

  const double delta_x = dist_x == 0 ? DBL_MAX : step_x / dist_x;
  const double delta_y = dist_y == 0 ? DBL_MAX : step_y / dist_y;

  double tmax_x = delta_x * (1.0 - fraction(begin.x));
  double tmax_y = delta_y * (1.0 - fraction(begin.y));

  double x = begin.x;
  double y = begin.y;

  vec2d* cells = NULL;
  sb_push(cells, begin);

  for (;;) {
    if (tmax_x < tmax_y) {
      tmax_x += delta_x;
      x += step_x;
    } else {
      tmax_y += delta_y;
      y += step_y;
    }

    vec2d p = {.x = x, .y = y};

    sb_push(cells, p);

    if (fabs(tmax_x) >= 1.0 && fabs(tmax_y) >= 1.0) break;
  }

  return cells;
}
