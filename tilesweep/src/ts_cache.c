#include "ts_cache.h"
#include <stdlib.h>
#include <string.h>
#include "platform.h"

static const uint8_t lfu_init = 5;

static ts_cache_slot* binary_search(ts_cache_slot* slots, int32_t len,
                                    uint64_t key) {
  int32_t begin = 0;
  int32_t end = len;

  int32_t steps = 0;
  while (begin < end) {
    int32_t m = (begin + end) / 2;
    steps++;
    uint64_t v = slots[m].item.key;
    if (v < key) {
      begin = m + 1;
    } else if (v > key) {
      end = m;
    } else {
      return &slots[m];
    }
  }

  return NULL;
}

static int32_t insertion_point(const ts_cache_slot* slots, int32_t len,
                               uint64_t key) {
  int32_t low = 0;
  int32_t high = len;

  while (low < high) {
    int32_t mid = (low + high) / 2;
    if (slots[mid].item.key < key)
      low = mid + 1;
    else
      high = mid;
  }

  return high;
}

ts_cache* ts_cache_create(const ts_cache_options* options) {
  const int32_t k_default_capacity = 256;

  ts_cache* cache = (ts_cache*)calloc(1, sizeof(ts_cache));
  cache->opts = *options;
  cache->time_seconds = seconds_now();
  cache->capacity = k_default_capacity;
  cache->slots = (ts_cache_slot*)calloc(cache->capacity, sizeof(ts_cache_slot));
  ts_rng_init(&cache->rng, ts_rng_seed());
  return cache;
}

static void decay(ts_cache_slot* slot, uint32_t time_now, uint32_t decay_time) {
  uint32_t diff = time_now - slot->last_access;
  uint32_t periods = diff / decay_time;

  uint8_t prev_freq = slot->freq;
  if (periods > slot->freq) {
    slot->freq = 0;
  } else {
    slot->freq -= periods;
  }

  if (prev_freq != slot->freq) {
    slot->last_access = time_now;
  }
}

static uint8_t increment_counter(ts_rng* rng, uint8_t counter) {
  const double lfu_log_factor = 10.0;

  if (counter == 255) {
    return 255;
  };

  uint32_t rnd = ts_rng_next(rng);
  double r = (double)rnd / 0xFFFFFFFF;
  double base = counter - lfu_init;

  if (base < 0) {
    base = 0;
  }

  double p = 1.0 / (base * lfu_log_factor + 1.0);

  if (r < p) {
    counter++;
  }

  return counter;
}

#include <inttypes.h>
void ts_cache_update(ts_cache* cache, uint32_t time_seconds) {
  cache->time_seconds = time_seconds;

  int j = 0;
  for (int32_t i = 0; i < cache->size; i++) {
    ts_cache_slot* slot = &cache->slots[i];
    decay(slot, cache->time_seconds, cache->opts.lfu_decay_time);

    if (slot->freq == 0) {
      cache->used_mem -= slot->item.size;
      if (cache->opts.on_item_expire) {
        cache->opts.on_item_expire(&slot->item);
      }
    } else {
      cache->slots[j++] = cache->slots[i];
    }
  }

  cache->size = j;
}

int32_t ts_cache_get(ts_cache* cache, uint64_t key, ts_cache_item* item) {
  ts_cache_slot* slot = binary_search(cache->slots, cache->size, key);

  if (!slot) {
    return 0;
  }

  slot->freq = increment_counter(&cache->rng, slot->freq);
  slot->last_access = cache->time_seconds;

  *item = slot->item;

  return 1;
}

static void accommodate(ts_cache_slot* slots, int32_t size, int32_t index) {
  for (int32_t i = size; i > index; i--) {
    slots[i] = slots[i - 1];
  }
}

int32_t ts_cache_set(ts_cache* cache, const ts_cache_item* item) {
  if (cache->used_mem + item->size > cache->opts.max_mem) {
    return 0;
  }

  if (cache->size == cache->capacity) {
    int32_t new_capacity = cache->capacity * 2;
    ts_cache_slot* new_slots =
        (ts_cache_slot*)calloc(new_capacity, sizeof(ts_cache_slot));
    memcpy(new_slots, cache->slots, cache->size * sizeof(ts_cache_slot));
    free(cache->slots);
    cache->slots = new_slots;
    cache->capacity = new_capacity;
  }

  int32_t insert_index = insertion_point(cache->slots, cache->size, item->key);
  accommodate(cache->slots, cache->size, insert_index);
  cache->size++;

  cache->slots[insert_index] = (ts_cache_slot){
      .item = *item, .last_access = cache->time_seconds, .freq = lfu_init};
  cache->used_mem += item->size;

  return 1;
}
