#pragma once

#include "tl_math.h"

typedef struct {
  vec2d* polygon;
  int32_t poly_len;
  vec2d* outline;
  int32_t outline_len;
  int32_t outline_idx;
  double x;
} fill_poly_state;

vec2d* fill_poly(const vec2d* poly, int32_t len);
void fill_poly_state_init(fill_poly_state* state, const vec2d* poly, int32_t len);
vec2d* fill_poly_advance(fill_poly_state* state, int32_t max_fills);
