#pragma once

#include <stdint.h>
#include <stddef.h>

struct tile {
  int w;
  int h;
  int x;
  int y;
  int z;

  uint64_t hash() const {
    return (uint64_t(z) << 40) | (uint64_t(x) << 20) | uint64_t(y);
  }

  bool valid() const { return w > -1 && h > -1 && x > -1 && y > -1 && z > -1; }
};

tile parse_tile(const char* s, size_t len);
