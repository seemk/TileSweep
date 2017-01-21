#pragma once

#include "tl_math.h"

typedef struct {
  double x;
  vec2d* polygon;
  vec2d* outline;
  int32_t poly_len;
  int32_t outline_len;
  int32_t inner_fill_idx;
  int32_t outline_idx;
  int32_t finished;
} fill_poly_state;

void fill_poly_state_init(fill_poly_state* state, const vec2d* poly, int32_t len);
void fill_poly_state_destroy(fill_poly_state* state);
vec2d* fill_poly_advance(fill_poly_state* state, int32_t max_fills);
