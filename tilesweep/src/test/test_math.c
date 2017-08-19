#include <stdio.h>
#include "../ts_math.h"
#include "minunit.h"

static const char* test_poly_area() {
  vec2d poly1[] = {{3, 4}, {5, 11}, {12, 8}, {9, 5}, {5, 6}};

  mu_assert("poly area", poly_area(poly1, 5) == 30.0);
  return NULL;
}

static const char* test_math() {
  mu_run_test(test_poly_area);
  return NULL;
}
