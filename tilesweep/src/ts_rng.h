#pragma once

#include <stdint.h>

typedef struct {
  uint32_t s[4];
} ts_rng;

uint64_t ts_rng_seed();
void ts_rng_init(ts_rng* state, uint32_t seed);
uint32_t ts_rng_next(ts_rng* state);
