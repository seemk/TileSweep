#include "miniunit.h"
#include <stdio.h>
#include "../hit_test.h"

const char* test_polygon_hit() {

  vec2i points[] = {
    { 4, 3 }, { 3, 4 }, { 3, 6 }, { 4, 7 },
    { 6, 7 }, { 7, 6 }, { 7, 4 }, { 6, 3 }
  };

  poly_hit_test t;
  poly_hit_test_init(&t, points, 8);

  mu_assert("out (3, 3)", !poly_hit_test_check(&t, vec2i{3, 3}));
  mu_assert("out (4, 3)", !poly_hit_test_check(&t, vec2i{4, 3}));
  mu_assert("in (5, 5)", poly_hit_test_check(&t, vec2i{5, 5}));
  mu_assert("in (6, 6)", poly_hit_test_check(&t, vec2i{6, 6}));

  return nullptr;
};

static const char* tests() {
  mu_run_test(test_polygon_hit);

  return nullptr;
}

int main(int argc, char** argv) {
  const char* res = tests();

  if (res != nullptr) {
    printf("failed: %s\n", res);
  }

  return res != nullptr;
}
