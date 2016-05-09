#pragma once

const double PI = 3.14159265358979323846;

template <typename T>
struct vec2 {
  T x;
  T y;
};

template <typename T>
struct vec3 {
  T x;
  T y;
  T z;
};

typedef vec2<double> vec2d;
typedef vec2<int> vec2i;
typedef vec3<double> vec3d;
typedef vec3<int> vec3i;

template <typename T>
T tl_clamp(T v, T min, T max) {
  if (v < min) return min;
  if (v > max) return max;

  return v;
}

inline double deg_to_rad(double degrees) {
  return degrees * PI / 180.0;
}

vec2d xyz_to_latlon(vec3d xyz);
vec2i latlon_to_xyz(vec2d coord, int zoom);

void latlon_to_xyz(const vec2d* coordinates, int len, int zoom, vec2i* out);
