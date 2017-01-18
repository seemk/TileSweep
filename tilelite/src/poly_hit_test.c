#include "poly_hit_test.h"
#include <stdlib.h>
#include <string.h>

void poly_hit_test_init(poly_hit_test* test, const vec2d* points, int32_t len) {
  test->poly_len = len;
  test->poly_points = (vec2d*)calloc(len, sizeof(vec2d));
  test->constants = (double*)calloc(len, sizeof(double));
  test->multiples = (double*)calloc(len, sizeof(double));

  memcpy(test->poly_points, points, len * sizeof(vec2d));

  int j = len - 1;
  for (int i = 0; i < len; i++) {
    vec2d pt_i = test->poly_points[i];
    vec2d pt_j = test->poly_points[j];
    if (pt_i.y == pt_j.y) {
      test->constants[i] = pt_i.x;
      test->multiples[i] = 0.0;
    } else {
      test->constants[i] = pt_i.x - (pt_i.y * pt_j.x) / (pt_j.y - pt_i.y) +
                           (pt_i.y * pt_i.x) / (pt_j.y - pt_i.y);
      test->multiples[i] = (pt_j.x - pt_i.x) / (pt_j.y - pt_i.y);
    }

    j = i;
  }
}

int poly_hit_test_check(poly_hit_test* test, double x, double y) {
  int j = test->poly_len - 1;

  int inside = 0;

  for (int32_t i = 0; i < test->poly_len; i++) {
    vec2d pt_i = test->poly_points[i];
    vec2d pt_j = test->poly_points[j];

    if ((pt_i.y < y && pt_j.y >= y) ||
        (pt_j.y < y && pt_i.y >= y)) {
      inside ^= (y * test->multiples[i] + test->constants[i] < x);
    }

    j = i;
  }

  return inside;
}

void poly_hit_test_destroy(poly_hit_test* test) {
  free(test->poly_points);
  free(test->constants);
  free(test->multiples);

  memset(test, 0, sizeof(poly_hit_test));
}
