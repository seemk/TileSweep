#include "tl_write_queue.h"
#include <pthread.h>
#include <stdlib.h>
#include "tc_queue.h"
#include "tc_sema.h"
#include "tl_log.h"
#include "tl_time.h"

struct tl_write_queue {
  tc_queue write_queue;
  int32_t full_threshold;
  pthread_t write_thread;
  pthread_mutex_t lock;
  tc_sema sema;
  image_db* db;
};

static void* commit_pending(void* arg) {
  tl_write_queue* q = (tl_write_queue*)arg;

  for (;;) {
    tc_sema_wait(&q->sema);

    pthread_mutex_lock(&q->lock);
    if (q->write_queue.length >= q->full_threshold) {
      tl_log("write queue full, begin clear");
      write_task task;
      while (tc_queue_pop(&q->write_queue, &task)) {
        if (image_db_add_image(q->db, &task.img, task.image_hash)) {
          image_db_add_position(q->db, tile_hash(&task.tile), task.image_hash);
        }
        free(task.img.data);
      }
      tl_log("write queue clear done");
    }

    write_task task;
    int32_t success = tc_queue_pop(&q->write_queue, &task);
    pthread_mutex_unlock(&q->lock);

    if (success) {
      if (image_db_add_image(q->db, &task.img, task.image_hash)) {
        image_db_add_position(q->db, tile_hash(&task.tile), task.image_hash);
      }
      free(task.img.data);
    }
  }
}

tl_write_queue* tl_write_queue_create(image_db* db) {
  tl_write_queue* q = (tl_write_queue*)calloc(1, sizeof(tl_write_queue));
  q->db = db;
  tc_queue_init(&q->write_queue, sizeof(write_task));
  q->full_threshold = 1024;
  pthread_mutex_init(&q->lock, NULL);
  tc_sema_init(&q->sema, 0);
  pthread_create(&q->write_thread, NULL, commit_pending, q);
  return q;
}

void tl_write_queue_push(tl_write_queue* q, tl_tile tile, image img,
                         uint64_t image_hash) {
  write_task task = {.tile = tile,
                     .img = img,
                     .image_hash = image_hash,
                     .start_time = tl_usec_now()};

  pthread_mutex_lock(&q->lock);
  tc_queue_push(&q->write_queue, &task);
  pthread_mutex_unlock(&q->lock);
  tc_sema_post(&q->sema, 1);
}

void tl_write_queue_destroy(tl_write_queue* q) { free(q); }
