#include "prerender.h"
#include <assert.h>
#include <limits.h>
#include <algorithm>

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

  return std::vector<vec2i>();
}
