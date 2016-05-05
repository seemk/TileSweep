#pragma once

#include <vector>

struct vec2i {
  int x;
  int y;
};

std::vector<vec2i> make_prerender_indices(const vec2i* poly, int len);
