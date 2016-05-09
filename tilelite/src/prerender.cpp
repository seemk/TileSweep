#include "prerender.h"
#include "hit_test.h"
#include <assert.h>
#include <limits.h>
#include <algorithm>
#include <unordered_set>

namespace {
  struct AABB {
    vec2i tl;
    vec2i br;
  };
}

std::vector<vec2i> make_prerender_indices(const vec2i* xyz_poly, int len) {

  if (len < 3) return std::vector<vec2i>();

  AABB bounds;
  bounds.tl.x = INT_MAX;
  bounds.tl.y = INT_MAX;
  bounds.br.x = INT_MIN;
  bounds.br.y = INT_MIN;

  for (int i = 0; i < len; i++) {
    vec2i pt = xyz_poly[i];
    
    if (pt.x < bounds.tl.x) bounds.tl.x = pt.x;
    if (pt.y < bounds.tl.y) bounds.tl.y = pt.y;
    if (pt.x > bounds.br.x) bounds.br.x = pt.x;
    if (pt.y > bounds.br.y) bounds.br.y = pt.y;
  }

  assert(bounds.tl.x != INT_MAX && bounds.tl.y != INT_MAX && bounds.br.x != INT_MIN && bounds.br.y != INT_MIN);

  std::vector<vec2i> indices;

  if (bounds.br.x - bounds.tl.x == 0 ||
      bounds.br.y - bounds.tl.y == 0) {
    for (int i = 0; i < len; i++) {
      indices.push_back(xyz_poly[i]);
    }

    return indices;
  }


  poly_hit_test test_ctx;
  poly_hit_test_init(&test_ctx, xyz_poly, len);
  for (int y = bounds.tl.y; y <= bounds.br.y; y++) {
    for (int x = bounds.tl.x; x <= bounds.br.x; x++) {
      bool inside = poly_hit_test_check(&test_ctx, vec2i{ x, y })
        || poly_hit_test_check(&test_ctx, vec2i{ x + 1, y })
        || poly_hit_test_check(&test_ctx, vec2i{ x, y + 1 })
        || poly_hit_test_check(&test_ctx, vec2i{ x + 1, y + 1 });
      if (inside) {
        indices.push_back(vec2i{x, y});
      }
    }
  }

  poly_hit_test_destroy(&test_ctx);

  return indices;
}
