#pragma once

#include "tl_math.h"

typedef struct {
  int32_t poly_len;
  vec2d* poly_points;
  double* constants;
  double* multiples;
} poly_hit_test;

void poly_hit_test_init(poly_hit_test* test, const vec2d* points, int32_t len);
int poly_hit_test_check(poly_hit_test* test, double x, double y);
void poly_hit_test_destroy(poly_hit_test* test);
