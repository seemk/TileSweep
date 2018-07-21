#include "ts_rng.h"
#include <x86intrin.h>

static inline uint32_t rotl(const uint32_t x, int k) {
  return (x << k) | (x >> (32 - k));
}

uint64_t ts_rng_seed() {
  uint64_t x = __rdtsc();
  uint64_t z = (x += UINT64_C(0x9E3779B97F4A7C15));
  z = (z ^ (z >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
  z = (z ^ (z >> 27)) * UINT64_C(0x94D049BB133111EB);
  return z ^ (z >> 31);
}

void ts_rng_init(ts_rng* state, uint32_t seed) {
  state->s[0] = seed;
  state->s[1] = seed;
  state->s[2] = seed;
  state->s[3] = seed;
}

uint32_t ts_rng_next(ts_rng* state) {
  uint32_t* s = &state->s[0];
  const uint32_t result_starstar = rotl(s[0] * 5, 7) * 9;

  const uint32_t t = s[1] << 9;

  s[2] ^= s[0];
  s[3] ^= s[1];
  s[1] ^= s[2];
  s[0] ^= s[3];

  s[2] ^= t;

  s[3] = rotl(s[3], 11);

  return result_starstar;
}

uint32_t rng_between(ts_rng* state, uint32_t low, uint32_t high) {
  uint32_t v = ts_rng_next(state);
  return low + v / (0xFFFFFFFF / (high - low + 1) + 1);
}
