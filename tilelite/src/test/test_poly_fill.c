#include <math.h>
#include <stdio.h>
#include <string.h>
#include "../poly_fill.h"
#include "../stretchy_buffer.h"
#include "../util.h"
#include "minunit.h"

static const vec2d concave1_expected[] = {
    {7.5, 7.5},   {22.5, 7.5},  {8.5, 8.5},   {9.5, 8.5},   {20.5, 8.5},
    {21.5, 8.5},  {9.5, 9.5},   {10.5, 9.5},  {11.5, 9.5},  {18.5, 9.5},
    {19.5, 9.5},  {20.5, 9.5},  {9.5, 10.5},  {10.5, 10.5}, {11.5, 10.5},
    {12.5, 10.5}, {13.5, 10.5}, {17.5, 10.5}, {18.5, 10.5}, {19.5, 10.5},
    {10.5, 11.5}, {11.5, 11.5}, {12.5, 11.5}, {13.5, 11.5}, {14.5, 11.5},
    {15.5, 11.5}, {16.5, 11.5}, {17.5, 11.5}, {18.5, 11.5}, {19.5, 11.5},
    {10.5, 12.5}, {11.5, 12.5}, {12.5, 12.5}, {13.5, 12.5}, {14.5, 12.5},
    {15.5, 12.5}, {16.5, 12.5}, {17.5, 12.5}, {18.5, 12.5}, {19.5, 12.5},
    {11.5, 13.5}, {12.5, 13.5}, {13.5, 13.5}, {14.5, 13.5}, {15.5, 13.5},
    {16.5, 13.5}, {17.5, 13.5}, {18.5, 13.5}, {19.5, 13.5}, {20.5, 13.5},
    {11.5, 14.5}, {12.5, 14.5}, {16.5, 14.5}, {17.5, 14.5}, {18.5, 14.5},
    {19.5, 14.5}, {20.5, 14.5}, {21.5, 14.5}, {10.5, 15.5}, {11.5, 15.5},
    {19.5, 15.5}, {20.5, 15.5}, {21.5, 15.5}, {9.5, 16.5},  {22.5, 16.5},
    {24.5, 4.5},  {25.5, 4.5},  {5.5, 5.5},   {6.5, 5.5},   {22.5, 5.5},
    {23.5, 5.5},  {24.5, 5.5},  {24.5, 5.5},  {25.5, 5.5},  {5.5, 6.5},
    {6.5, 6.5},   {6.5, 6.5},   {7.5, 6.5},   {8.5, 6.5},   {21.5, 6.5},
    {22.5, 6.5},  {23.5, 6.5},  {24.5, 6.5},  {6.5, 7.5},   {8.5, 7.5},
    {9.5, 7.5},   {10.5, 7.5},  {19.5, 7.5},  {20.5, 7.5},  {21.5, 7.5},
    {23.5, 7.5},  {6.5, 8.5},   {7.5, 8.5},   {10.5, 8.5},  {11.5, 8.5},
    {12.5, 8.5},  {17.5, 8.5},  {18.5, 8.5},  {19.5, 8.5},  {22.5, 8.5},
    {23.5, 8.5},  {7.5, 9.5},   {8.5, 9.5},   {12.5, 9.5},  {13.5, 9.5},
    {14.5, 9.5},  {16.5, 9.5},  {17.5, 9.5},  {21.5, 9.5},  {22.5, 9.5},
    {8.5, 10.5},  {14.5, 10.5}, {15.5, 10.5}, {16.5, 10.5}, {20.5, 10.5},
    {21.5, 10.5}, {8.5, 11.5},  {9.5, 11.5},  {20.5, 11.5}, {9.5, 12.5},
    {20.5, 12.5}, {21.5, 12.5}, {9.5, 13.5},  {10.5, 13.5}, {21.5, 13.5},
    {22.5, 13.5}, {9.5, 14.5},  {10.5, 14.5}, {13.5, 14.5}, {14.5, 14.5},
    {15.5, 14.5}, {22.5, 14.5}, {8.5, 15.5},  {9.5, 15.5},  {12.5, 15.5},
    {13.5, 15.5}, {15.5, 15.5}, {16.5, 15.5}, {17.5, 15.5}, {18.5, 15.5},
    {22.5, 15.5}, {23.5, 15.5}, {7.5, 16.5},  {8.5, 16.5},  {10.5, 16.5},
    {11.5, 16.5}, {12.5, 16.5}, {18.5, 16.5}, {19.5, 16.5}, {20.5, 16.5},
    {21.5, 16.5}, {23.5, 16.5}, {24.5, 16.5}, {6.5, 17.5},  {7.5, 17.5},
    {8.5, 17.5},  {9.5, 17.5},  {10.5, 17.5}, {21.5, 17.5}, {22.5, 17.5},
    {23.5, 17.5}, {24.5, 17.5}, {24.5, 17.5}, {25.5, 17.5}, {5.5, 18.5},
    {6.5, 18.5},  {7.5, 18.5},  {8.5, 18.5},  {24.5, 18.5}, {25.5, 18.5},
    {4.5, 19.5},  {5.5, 19.5},  {5.5, 19.5},  {6.5, 19.5},  {7.5, 19.5},
    {4.5, 20.5},  {5.5, 20.5}};

static const vec2d concave1[] = {{5.5, 5.5},   {15.5, 10.5}, {25.5, 4.5},
                                 {20.5, 11.5}, {25.5, 18.5}, {14.5, 14.5},
                                 {4.5, 20.5},  {10.5, 14.5}};
static const vec2d convex1[] = {{0.5, 0.5}, {5.5, 0.5}, {5.5, 5.5}, {0.5, 5.5}};
static const vec2d convex2[] = {
    {5.5, 5.5}, {10.5, 5.5}, {10.5, 10.5}, {5.5, 10.5}};
static const vec2d singular = {5.5, 5.5};

static const vec2d convex1_expected[] = {
    {1.5, 1.5}, {2.5, 1.5}, {3.5, 1.5}, {4.5, 1.5}, {1.5, 2.5}, {2.5, 2.5},
    {3.5, 2.5}, {4.5, 2.5}, {1.5, 3.5}, {2.5, 3.5}, {3.5, 3.5}, {4.5, 3.5},
    {1.5, 4.5}, {2.5, 4.5}, {3.5, 4.5}, {4.5, 4.5}, {0.5, 0.5}, {1.5, 0.5},
    {2.5, 0.5}, {3.5, 0.5}, {4.5, 0.5}, {5.5, 0.5}, {0.5, 1.5}, {5.5, 1.5},
    {0.5, 2.5}, {5.5, 2.5}, {0.5, 3.5}, {5.5, 3.5}, {0.5, 4.5}, {5.5, 4.5},
    {0.5, 5.5}, {1.5, 5.5}, {2.5, 5.5}, {3.5, 5.5}, {4.5, 5.5}, {5.5, 5.5}};

static const vec2d convex2_expected[] = {
    {6.5, 6.5},  {7.5, 6.5},  {8.5, 6.5},  {9.5, 6.5},  {6.5, 7.5},
    {7.5, 7.5},  {8.5, 7.5},  {9.5, 7.5},  {6.5, 8.5},  {7.5, 8.5},
    {8.5, 8.5},  {9.5, 8.5},  {6.5, 9.5},  {7.5, 9.5},  {8.5, 9.5},
    {9.5, 9.5},  {5.5, 5.5},  {6.5, 5.5},  {7.5, 5.5},  {8.5, 5.5},
    {9.5, 5.5},  {10.5, 5.5}, {5.5, 6.5},  {10.5, 6.5}, {5.5, 7.5},
    {10.5, 7.5}, {5.5, 8.5},  {10.5, 8.5}, {5.5, 9.5},  {10.5, 9.5},
    {5.5, 10.5}, {6.5, 10.5}, {7.5, 10.5}, {8.5, 10.5}, {9.5, 10.5},
    {10.5, 10.5}};

typedef struct {
  const vec2d* poly;
  int32_t poly_len;
  const vec2d* expected;
  int32_t expected_len;
  int32_t batch_size;
  const char* name;
} fill_test;

fill_test tests[] = {
  {concave1, countof(concave1), concave1_expected, countof(concave1_expected), 5, "concave"},
  {convex1, countof(convex1), convex1_expected, countof(convex1_expected), 5, "convex"},
  {&singular, 1, &singular, 1, 5, "singular"},
  {convex2, countof(convex2), convex2_expected, countof(convex2_expected), 3},
  {convex2, countof(convex2), convex2_expected, countof(convex2_expected), 1},
  {convex2, countof(convex2), convex2_expected, countof(convex2_expected), 40}
};

static const char* poly_fill_stateful(const vec2d* poly, int32_t poly_len,
                                      const vec2d* expected,
                                      int32_t expected_len, int32_t batch_size,
                                      const char* name) {
  fill_poly_state state;
  fill_poly_state_init(&state, poly, poly_len);

  int32_t batch_bytes = batch_size * sizeof(vec2d);
  int32_t total_batches =
      (int32_t)ceil((double)expected_len / (double)batch_size);

  for (int32_t i = 0; i < total_batches - 1; i++) {
    vec2d* filled = fill_poly_advance(&state, batch_size);
    mu_assert(name, sb_count(filled) == batch_size);
    mu_assert(name, !memcmp(filled, expected + i * batch_size, batch_bytes));

    sb_free(filled);
  }

  int32_t remain = expected_len % batch_size;
  vec2d* filled = fill_poly_advance(&state, batch_size);
  mu_assert(name, sb_count(filled) == remain);
  mu_assert(name, !memcmp(filled, &expected[batch_size * (total_batches - 1)],
                          remain * sizeof(vec2d)));
  mu_assert(name, state.finished);
  sb_free(filled);

  return NULL;
}

static const char* test_poly_fill() {
  for (int32_t i = 0; i < countof(tests); i++) {
    const fill_test* t = &tests[i];
    const char* res =
        poly_fill_stateful(t->poly, t->poly_len, t->expected, t->expected_len,
                           t->batch_size, t->name);
    if (res) {
      return res;
    }
  }
  return NULL;
}
