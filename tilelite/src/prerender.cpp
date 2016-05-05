#include "prerender.h"
#include <algorithm>

namespace {
  struct AABB {
    vec2i tl;
    vec2i br;
  };
}

std::vector<vec2i> make_prerender_indices(const vec2i* poly, int len) {

  AABB bounds = { 0 };
  for (int i = 0; i < len; i++) {

  }

  return std::vector<vec2i>();
}
