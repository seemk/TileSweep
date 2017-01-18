#include "poly_fill.h"
#include <assert.h>
#include <float.h>
#include <math.h>
#include "poly_hit_test.h"
#include "stretchy_buffer.h"

static int line_cmp(const void* a, const void* b) {
  const vec2d* p1 = (const vec2d*)a;
  const vec2d* p2 = (const vec2d*)b;

  if (p1->y == p2->y) {
    return p1->x - p2->x;
  }

  return p1->y - p2->y;
}

static inline double fraction(double x) { return x - floor(x); }

static vec2d* make_outline(const vec2d* poly, int32_t len) {
  vec2d* outline = NULL;

  vec2d begin = poly[0];
  vec2d end = poly[len - 1];

  for (int32_t i = 0; i < len - 1; i++) {
    raycast(poly[i], poly[i + 1], outline);
  }

  raycast(end, begin, outline);

  return outline;
}

void raycast(vec2d begin, vec2d end, vec2d* out) {
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

  sb_push(out, begin);

  for (;;) {
    if (tmax_x < tmax_y) {
      tmax_x += delta_x;
      x += step_x;
    } else {
      tmax_y += delta_y;
      y += step_y;
    }

    vec2d p = {.x = x, .y = y};

    sb_push(out, p);

    if (fabs(tmax_x) >= 1.0 && fabs(tmax_y) >= 1.0) break;
  }
}

vec2d* fill_poly(const vec2d* poly, int32_t len) {
  assert(len > 0);
  vec2d* outline = make_outline(poly, len);
  vec2d* filled = NULL;

  qsort(outline, sb_count(outline), sizeof(vec2d), line_cmp);

  poly_hit_test test;
  poly_hit_test_init(&test, outline, sb_count(outline));

  for (int32_t i = 0; i < sb_count(outline) - 1; i++) {
    vec2d p = outline[i];
    vec2d n = outline[i + 1];

    if (p.y == n.y) {
      for (int32_t x = p.x + 1; x < n.x; x++) {
        if (poly_hit_test_check(&test, (double)x, p.y)) {
          vec2d in = {.x = (double)x, p.y};
          sb_push(filled, in);
        }
      }
    }
  }

  for (int32_t i = 0; i < sb_count(outline); i++) {
    sb_push(filled, outline[i]);
  }

  poly_hit_test_destroy(&test);
  sb_free(outline);

  return filled;
}
