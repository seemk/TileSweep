#pragma once

#include "tl_math.h"

struct poly_hit_test {
  int poly_len;
  vec2d* poly_points;
  double* constants;
  double* multiples;
};

void poly_hit_test_init(poly_hit_test* test, const vec2i* points, int len);
bool poly_hit_test_check(poly_hit_test* test, vec2i point);
void poly_hit_test_destroy(poly_hit_test* test);
