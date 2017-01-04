#include "prerender.h"
#include <assert.h>
#include <limits.h>
#include "poly_hit_test.h"
#include "stretchy_buffer.h"

vec2i* make_prerender_indices(const vec2i* xyz_poly, int len) {
  if (len < 3) return NULL;

  vec2i top_left = {.x = INT_MAX, .y = INT_MAX};

  vec2i bot_right = {.x = -1, .y = -1};

  for (int i = 0; i < len; i++) {
    vec2i pt = xyz_poly[i];

    if (pt.x < top_left.x) top_left.x = pt.x;
    if (pt.y < top_left.y) top_left.y = pt.y;
    if (pt.x > bot_right.x) bot_right.x = pt.x;
    if (pt.y > bot_right.y) bot_right.y = pt.y;
  }

  assert(top_left.x != INT_MAX && top_left.y != INT_MAX && bot_right.x != -1 &&
         bot_right.y != -1);

  vec2i* indices = NULL;

  if (bot_right.x - top_left.x == 0 || bot_right.y - top_left.y == 0) {
    for (int i = 0; i < len; i++) {
      sb_push(indices, xyz_poly[i]);
    }

    return indices;
  }

  poly_hit_test test_ctx;
  poly_hit_test_init(&test_ctx, xyz_poly, len);
  for (int y = top_left.y; y <= bot_right.y; y++) {
    for (int x = top_left.x; x <= bot_right.x; x++) {
      vec2i pt = {.x = x, .y = y};

      int inside =
          poly_hit_test_check(&test_ctx, pt) ||
          poly_hit_test_check(&test_ctx, (vec2i){.x = x + 1, .y = y}) ||
          poly_hit_test_check(&test_ctx, (vec2i){.x = x, .y = y + 1}) ||
          poly_hit_test_check(&test_ctx, (vec2i){.x = x + 1, .y = y + 1});
      if (inside) {
        sb_push(indices, pt);
      }
    }
  }

  poly_hit_test_destroy(&test_ctx);

  return indices;
}
