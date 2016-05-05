#pragma once

template <typename T>
T tl_clamp(T v, T min, T max) {
  if (v < min) return min;
  if (v > max) return max;

  return v;
}
