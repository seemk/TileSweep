#pragma once

#include "tl_math.h"
#include <vector>

std::vector<vec2i> make_prerender_indices(const vec2i* xyz_poly, int len);
