#pragma once

#include <stdint.h>

struct tile {
  int z;
  int x;
  int y;
  int w;
  int h;
  int64_t request_time;

  uint64_t hash() {
    return (uint64_t(z) << 40) | (uint64_t(x) << 20) | uint64_t(y);
  }
};

tile parse_tile(const char* s, int len);
