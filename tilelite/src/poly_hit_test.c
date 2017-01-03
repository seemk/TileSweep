#include "poly_hit_test.h"
#include <stdlib.h>
#include <string.h>

void poly_hit_test_init(poly_hit_test* test, const vec2i* points, int len) {
  test->poly_len = len;
  test->poly_points = (vec2d*)calloc(len, sizeof(vec2d));
  test->constants = (double*)calloc(len, sizeof(double));
  test->multiples = (double*)calloc(len, sizeof(double));

  for (int i = 0; i < len; i++) {
    vec2i pt = points[i];
    test->poly_points[i] = (vec2d){.x = (double)pt.x, .y = (double)pt.y};
  }

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

int poly_hit_test_check(poly_hit_test* test, vec2i point) {
  int j = test->poly_len - 1;

  int inside = 0;

  vec2d pt = {.x = (double)point.x, .y = (double)point.y};

  for (int i = 0; i < test->poly_len; i++) {
    vec2d pt_i = test->poly_points[i];
    vec2d pt_j = test->poly_points[j];

    if ((pt_i.y < pt.y && pt_j.y >= pt.y) ||
        (pt_j.y < pt.y && pt_i.y >= pt.y)) {
      inside ^= (pt.y * test->multiples[i] + test->constants[i] < pt.x);
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
