#pragma once
#include "image_db.h"
#include "tl_tile.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct tl_write_queue tl_write_queue;
typedef struct {
  tl_tile tile;
  image img;
  uint64_t image_hash;
  int64_t start_time;
} write_task;

tl_write_queue* tl_write_queue_create(image_db* db);
void tl_write_queue_push(tl_write_queue* q, tl_tile t, image img,
                         uint64_t image_hash);
void tl_write_queue_destroy(tl_write_queue* q);

#ifdef __cplusplus
}
#endif
