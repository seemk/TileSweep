#pragma once

#include <stdint.h>
#include "ts_rng.h"

typedef struct {
  uint64_t key;
  void* user;
  int32_t size;
} ts_cache_item;

typedef struct {
  ts_cache_item item;
  uint32_t last_access;
  uint32_t freq;
} ts_cache_slot;

typedef struct {
  uint64_t max_mem;
  double lfu_log_factor;
  uint32_t lfu_decay_time;
  void (*on_item_expire)(const ts_cache_item*);
} ts_cache_options;

typedef struct {
  ts_cache_options opts;
  uint64_t used_mem;
  uint32_t time_seconds;
  int32_t capacity;
  int32_t size;

  ts_cache_slot* slots;
  ts_rng rng;
} ts_cache;

ts_cache* ts_cache_create(const ts_cache_options* options);
void ts_cache_update(ts_cache* cache, uint32_t time_seconds);
int32_t ts_cache_get(ts_cache* cache, uint64_t key, ts_cache_item* item);
int32_t ts_cache_set(ts_cache* cache, const ts_cache_item* item);
