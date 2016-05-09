#pragma once

#include <vector>

template <typename T>
struct vec2 {
  T x;
  T y;
};

typedef vec2<double> vec2d;
typedef vec2<int> vec2i;

std::vector<vec2i> make_prerender_indices(const vec2i* xyz_poly, int len);
