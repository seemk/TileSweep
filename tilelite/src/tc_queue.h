#pragma once

#include <stdint.h>

typedef struct {
  int32_t item_size;
  int32_t start;
  int32_t length;
  int32_t capacity;
  uint8_t* items;
} tc_queue;

tc_queue* tc_queue_create(int32_t item_size);
void tc_queue_init(tc_queue* q, int32_t item_size);
void tc_queue_push(tc_queue* q, const void* item);
int32_t tc_queue_pop(tc_queue* q, void* item);
