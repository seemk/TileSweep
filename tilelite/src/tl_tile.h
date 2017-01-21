#pragma once

#include <stddef.h>
#include <stdint.h>

typedef struct {
  int32_t x;
  int32_t y;
  int32_t z;
  int32_t w;
  int32_t h;
} tl_tile;

static inline uint64_t tile_hash(const tl_tile* t) {
  return ((uint64_t)t->z << 40) | ((uint64_t)t->x << 20) | (uint64_t)t->y;
}

int32_t tile_valid(const tl_tile* t);

tl_tile parse_tile(const char* s, size_t len);
