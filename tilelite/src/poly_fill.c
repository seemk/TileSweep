#include "poly_fill.h"
#include <assert.h>
#include <float.h>
#include <math.h>
#include <string.h>
#include "poly_hit_test.h"
#include "stretchy_buffer.h"
#include "tl_math.h"

static inline double frac_pos(double x) { return x - floor(x); }
static inline double frac_neg(double x) { return 1.0 - x + floor(x); }

void raycast(vec2d begin, vec2d end, vec2d** out) {
  const double dist_x = end.x - begin.x;
  const double dist_y = end.y - begin.y;

  const double step_x = sign(dist_x);
  const double step_y = sign(dist_y);

  const double delta_x = dist_x == 0 ? DBL_MAX : step_x / dist_x;
  const double delta_y = dist_y == 0 ? DBL_MAX : step_y / dist_y;

  double tmax_x = step_x > 0.0 ? delta_x * frac_neg(begin.x) : delta_x * frac_pos(begin.x);
  double tmax_y = step_y > 0.0 ? delta_y * frac_neg(begin.y) : delta_y * frac_pos(begin.y);

  double x = floor(begin.x);
  double y = floor(begin.y);

  vec2d initial = {.x = x, .y = y};
  sb_push(*out, initial);

  for (;;) {
    if (tmax_x < tmax_y) {
      tmax_x += delta_x;
      x += step_x;
    } else {
      tmax_y += delta_y;
      y += step_y;
    }

    if (tmax_x > 1.0 && tmax_y > 1.0) break;

    vec2d p = {.x = x, .y = y};
    sb_push(*out, p);

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

static vec2d* unique(vec2d* a, int32_t len) {
  vec2d* uniques = NULL;
  
  if (len == 0) {
    return uniques;
  }

  qsort(a, len, sizeof(vec2d), line_cmp);

  sb_push(uniques, a[0]);
  for (int32_t i = 1; i < len - 1; i++) {
    vec2d p = a[i - 1];
    vec2d n = a[i];

    if (p.x != n.x || p.y != n.y) {
      sb_push(uniques, n);
    }
  }

  return uniques;
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
          vec2d* filled_unique = unique(filled, sb_count(filled));
          sb_free(filled);
          return filled_unique;
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

  state->finished = 1;

  poly_hit_test_destroy(&test);
  vec2d* filled_unique = unique(filled, sb_count(filled));
  sb_free(filled);

  return filled_unique;
}

void fill_poly_state_destroy(fill_poly_state* state) {
  free(state->polygon);
  free(state->outline);
}
