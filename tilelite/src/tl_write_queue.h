#pragma once
#include <pthread.h>
#include "concurrentqueue.h"
#include "sema.h"
#include "image.h"
#include "tl_tile.h"
#include "image_db.h"

struct write_task {
  tl_tile tile;
  image img;
  uint64_t image_hash;
  int64_t start_time;
};

template <typename T>
using tl_queue = moodycamel::ConcurrentQueue<T>;

typedef tl_queue<write_task> tl_tile_queue;

struct tl_write_queue {
  pthread_t write_thread;
  LightweightSemaphore sema;
  tl_tile_queue* write_queue;
  image_db* db;
};

void tl_write_queue_init(tl_write_queue* q, image_db* db);
void tl_write_queue_push(tl_write_queue* q, tl_tile t, image img, uint64_t image_hash);
void tl_write_queue_destroy(tl_write_queue* q);
