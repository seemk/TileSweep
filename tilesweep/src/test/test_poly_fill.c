#include <math.h>
#include <stdio.h>
#include <string.h>
#include "../poly_fill.h"
#include "../stretchy_buffer.h"
#include "../util.h"
#include "minunit.h"

static void print_coords(const vec2d* a, int32_t len) {
  for (int32_t j = 0; j < len; j++) {
    printf("[%5.2f, %5.2f], ", a[j].x, a[j].y);
    if ((j + 1) % 5 == 0) {
      printf("\n");
    }
  }
  printf("\n");
}

static int line_cmp(const void* a, const void* b) {
  const vec2d* p1 = (const vec2d*)a;
  const vec2d* p2 = (const vec2d*)b;

  if (p1->y == p2->y) {
    return p1->x - p2->x;
  }

  return p1->y - p2->y;
}

static const vec2d concave1[] = {{5.6, 6},   {16.6, 10.8}, {25.5, 4.7},
                                 {20.1, 12}, {25.2, 18.9}, {14.6, 14.6},
                                 {4.1, 20},  {10.1, 14.7}};

static const vec2d convex1[] = {{0.8, 0.5}, {5.3, 0.8}, {5.8, 5.5}, {0.3, 5.9}};
static const vec2d convex3[] = {
    {4654.5, 2450.5}, {4654.5, 2451.5}, {4655.5, 2450.5}};

static const vec2d singular[] = {{5.5, 5.4}};

static const vec2d singular_expected[] = {{5, 5}};

static const vec2d concave1_expected[] = {
    {25, 4},  {23, 5},  {24, 5},  {25, 5},  {5, 6},   {6, 6},   {7, 6},
    {22, 6},  {23, 6},  {24, 6},  {6, 7},   {7, 7},   {8, 7},   {9, 7},
    {10, 7},  {20, 7},  {21, 7},  {22, 7},  {23, 7},  {6, 8},   {7, 8},
    {8, 8},   {9, 8},   {10, 8},  {11, 8},  {12, 8},  {19, 8},  {20, 8},
    {21, 8},  {22, 8},  {23, 8},  {7, 9},   {8, 9},   {9, 9},   {10, 9},
    {11, 9},  {12, 9},  {13, 9},  {14, 9},  {17, 9},  {18, 9},  {19, 9},
    {20, 9},  {21, 9},  {22, 9},  {7, 10},  {8, 10},  {9, 10},  {10, 10},
    {11, 10}, {12, 10}, {13, 10}, {14, 10}, {15, 10}, {16, 10}, {17, 10},
    {18, 10}, {19, 10}, {20, 10}, {21, 10}, {8, 11},  {9, 11},  {10, 11},
    {11, 11}, {12, 11}, {13, 11}, {14, 11}, {15, 11}, {16, 11}, {17, 11},
    {18, 11}, {19, 11}, {20, 11}, {8, 12},  {9, 12},  {10, 12}, {11, 12},
    {12, 12}, {13, 12}, {14, 12}, {15, 12}, {16, 12}, {17, 12}, {18, 12},
    {19, 12}, {20, 12}, {9, 13},  {10, 13}, {11, 13}, {12, 13}, {13, 13},
    {14, 13}, {15, 13}, {16, 13}, {17, 13}, {18, 13}, {19, 13}, {20, 13},
    {21, 13}, {9, 14},  {10, 14}, {11, 14}, {12, 14}, {13, 14}, {14, 14},
    {15, 14}, {16, 14}, {17, 14}, {18, 14}, {19, 14}, {20, 14}, {21, 14},
    {22, 14}, {8, 15},  {9, 15},  {10, 15}, {11, 15}, {12, 15}, {13, 15},
    {15, 15}, {16, 15}, {17, 15}, {18, 15}, {19, 15}, {20, 15}, {21, 15},
    {22, 15}, {23, 15}, {7, 16},  {8, 16},  {9, 16},  {10, 16}, {11, 16},
    {18, 16}, {19, 16}, {20, 16}, {21, 16}, {22, 16}, {23, 16}, {6, 17},
    {7, 17},  {8, 17},  {9, 17},  {20, 17}, {21, 17}, {22, 17}, {23, 17},
    {24, 17}, {5, 18},  {6, 18},  {7, 18},  {22, 18}, {23, 18}, {24, 18},
    {25, 18}, {4, 19},  {5, 19},  {6, 19},  {4, 20}};

static const vec2d convex1_expected[] = {
    {0, 0}, {1, 0}, {2, 0}, {3, 0}, {4, 0}, {5, 0}, {0, 1}, {1, 1}, {2, 1},
    {3, 1}, {4, 1}, {5, 1}, {0, 2}, {1, 2}, {2, 2}, {3, 2}, {4, 2}, {5, 2},
    {0, 3}, {1, 3}, {2, 3}, {3, 3}, {4, 3}, {5, 3}, {0, 4}, {1, 4}, {2, 4},
    {3, 4}, {4, 4}, {5, 4}, {0, 5}, {1, 5}, {2, 5}, {3, 5}, {4, 5}, {5, 5}};

typedef struct {
  const vec2d* poly;
  int32_t poly_len;
  const vec2d* expected;
  int32_t expected_len;
  int32_t batch_size;
  const char* name;
} fill_test;

fill_test tests[] = {{concave1, countof(concave1), concave1_expected,
                      countof(concave1_expected), 100, "concave"},
                     {singular, 1, singular_expected, 1, 5, "singular"},
                     {convex1, countof(convex1), convex1_expected,
                      countof(convex1_expected), 5, "convex 1"}};

static const char* poly_fill_stateful(const vec2d* poly, int32_t poly_len,
                                      const vec2d* expected,
                                      int32_t expected_len, int32_t batch_size,
                                      const char* name) {
  fill_poly_state state;
  fill_poly_state_init(&state, poly, poly_len);

  int32_t total_batches =
      (int32_t)ceil((double)expected_len / (double)batch_size);

  vec2d* result = NULL;

  for (int32_t i = 0; i < total_batches - 1; i++) {
    vec2d* filled = fill_poly_advance(&state, batch_size);

    for (int32_t j = 0; j < sb_count(filled); j++) {
      sb_push(result, filled[j]);
    }

    sb_free(filled);
  }

  while (!state.finished) {
    vec2d* filled = fill_poly_advance(&state, batch_size);
    for (int32_t j = 0; j < sb_count(filled); j++) {
      sb_push(result, filled[j]);
    }
    sb_free(filled);
  }

  qsort(result, sb_count(result), sizeof(vec2d), line_cmp);

  if (sb_count(result) != expected_len) {
    printf("%d fills, expected %d\n", sb_count(result), expected_len);
    print_coords(result, sb_count(result));
    return name;
  }

  for (int32_t i = 0; i < expected_len; i++) {
    if (expected[i].x != result[i].x || expected[i].y != result[i].y) {
      printf("mismatch at index %d. expected: (%.1f, %.1f), got (%.1f, %.1f)\n",
             i, expected[i].x, expected[i].y, result[i].x, result[i].y);

      print_coords(result, expected_len);
      return name;
    }
  }

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
