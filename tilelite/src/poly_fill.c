#include "poly_fill.h"
#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "poly_hit_test.h"
#include "stretchy_buffer.h"
#include "tl_math.h"

static inline double fraction(double x) { return x - floor(x); }

void raycast(vec2d begin, vec2d end, vec2d** out) {
  const double dist_x = end.x - begin.x;
  const double dist_y = end.y - begin.y;

  const double step_x = sign(dist_x);
  const double step_y = sign(dist_y);

  const double delta_x = dist_x == 0 ? DBL_MAX : step_x / dist_x;
  const double delta_y = dist_y == 0 ? DBL_MAX : step_y / dist_y;

  double tmax_x = delta_x * (1.0 - fraction(begin.x));
  double tmax_y = delta_y * (1.0 - fraction(begin.y));

  double x = begin.x;
  double y = begin.y;

  for (;;) {
    if (tmax_x < tmax_y) {
      tmax_x += delta_x;
      x += step_x;
    } else {
      tmax_y += delta_y;
      y += step_y;
    }

    vec2d p = {.x = x, .y = y};

    sb_push(*out, p);

    if (fabs(tmax_x) >= 1.0 && fabs(tmax_y) >= 1.0) break;
  }
}

static int line_cmp(const void* a, const void* b) {
  const vec2d* p1 = (const vec2d*)a;
  const vec2d* p2 = (const vec2d*)b;

  if (p1->y == p2->y) {
    return p1->x - p2->x;
  }

  return p1->y - p2->y;
}

static vec2d* make_outline(const vec2d* poly, int32_t len) {
  vec2d* outline = NULL;

  vec2d begin = poly[0];
  vec2d end = poly[len - 1];

  for (int32_t i = 0; i < len - 1; i++) {
    raycast(poly[i], poly[i + 1], &outline);
  }

  raycast(end, begin, &outline);

  return outline;
}

void fill_poly_state_init(fill_poly_state* state, const vec2d* poly,
                          int32_t len) {
  assert(len > 0);
  memset(state, 0, sizeof(fill_poly_state));
  state->polygon = (vec2d*)calloc(len, sizeof(vec2d));
  state->poly_len = len;

  memcpy(state->polygon, poly, len * sizeof(vec2d));

  vec2d* outline = make_outline(poly, len);

  qsort(outline, sb_count(outline), sizeof(vec2d), line_cmp);

  state->outline = (vec2d*)calloc(sb_count(outline), sizeof(vec2d));
  state->outline_len = sb_count(outline);
  memcpy(state->outline, outline, sb_count(outline) * sizeof(vec2d));

  sb_free(outline);
}

vec2d* fill_poly_advance(fill_poly_state* state, int32_t max_fills) {
  vec2d* filled = NULL;

  poly_hit_test test;
  poly_hit_test_init(&test, state->polygon, state->poly_len);

  for (int32_t i = state->inner_fill_idx; i < state->outline_len - 1; i++) {
    vec2d p = state->outline[i];
    vec2d n = state->outline[i + 1];

    if (p.y == n.y) {
      for (double x = p.x + 1.0 + state->x; x < n.x; x += 1.0) {
        if (poly_hit_test_check(&test, x, p.y)) {
          vec2d in = {.x = x, .y = p.y};
          sb_push(filled, in);
        }

        state->x++;

        if (sb_count(filled) >= max_fills) {
          poly_hit_test_destroy(&test);
          return filled;
        }
      }
    }

    state->x = 0.0;
    state->inner_fill_idx++;
  }

  while (sb_count(filled) < max_fills &&
         state->outline_idx < state->outline_len) {
    sb_push(filled, state->outline[state->outline_idx++]);
  }

  poly_hit_test_destroy(&test);

  state->finished = 1;

  return filled;
}
