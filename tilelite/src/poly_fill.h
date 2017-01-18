#pragma once

#include "tl_math.h"

void raycast(vec2d begin, vec2d end, vec2d* out);
vec2d* fill_poly(const vec2d* poly, int32_t len);
