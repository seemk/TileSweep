#pragma once
#include "image_db.h"
#include "ts_tile.h"

typedef struct {
  ts_tile tile;
  image img;
  uint64_t image_hash;
  int64_t start_time;
} write_task;

typedef struct ts_write_queue ts_write_queue;

ts_write_queue* ts_write_queue_create(image_db* db);
void ts_write_queue_push(ts_write_queue* q, ts_tile t, image img,
                         uint64_t image_hash);
void ts_write_queue_destroy(ts_write_queue* q);
void ts_write_queue_commit(ts_write_queue* q);
