#pragma once

#include <stddef.h>
#include <stdint.h>

typedef struct {
  int32_t x;
  int32_t y;
  int32_t z;
  int32_t w;
  int32_t h;
} ts_tile;

static inline uint64_t tile_hash(const ts_tile* t) {
  return ((uint64_t)t->z << 40) | ((uint64_t)t->x << 20) | (uint64_t)t->y;
}

int32_t tile_valid(const ts_tile* t);

ts_tile parse_tile(const char* s, size_t len);
