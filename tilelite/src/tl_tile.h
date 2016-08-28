#pragma once

#include <stdint.h>
#include <stddef.h>

struct tl_tile {
  int x;
  int y;
  int z;
  int w;
  int h;

  uint64_t hash() const {
    return (uint64_t(z) << 40) | (uint64_t(x) << 20) | uint64_t(y);
  }

  bool valid() const { return w > 0 && h > 0 && x > -1 && y > -1 && z > -1; }
};

tl_tile parse_tile(const char* s, size_t len);
