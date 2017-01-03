#pragma once

#include "tl_math.h"

typedef struct {
  int poly_len;
  vec2d* poly_points;
  double* constants;
  double* multiples;
} poly_hit_test;

void poly_hit_test_init(poly_hit_test* test, const vec2i* points, int len);
int poly_hit_test_check(poly_hit_test* test, vec2i point);
void poly_hit_test_destroy(poly_hit_test* test);
