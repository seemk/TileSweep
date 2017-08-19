#pragma once

#include <stdint.h>

typedef struct {
  int32_t item_size;
  int32_t start;
  int32_t length;
  int32_t capacity;
  uint8_t* items;
} ts_queue;

ts_queue* ts_queue_create(int32_t item_size);
void ts_queue_init(ts_queue* q, int32_t item_size);
void ts_queue_push(ts_queue* q, const void* item);
int32_t ts_queue_pop(ts_queue* q, void* item);
