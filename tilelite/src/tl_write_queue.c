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
  int32_t insert_threshold;
  int32_t inserts_len;
  image_db_insert* inserts;
  pthread_t write_thread;
  pthread_mutex_t lock;
  tc_sema sema;
  image_db* db;
};

static void populate_pending_inserts(tl_write_queue* q, int32_t num) {
  for (int32_t i = 0; i < num; i++) {
    write_task task;
    if (tc_queue_pop(&q->write_queue, &task)) {
      image_db_insert* ins = &q->inserts[q->inserts_len++];
      ins->img = task.img;
      ins->image_hash = task.image_hash;
      ins->position_hash = tile_hash(&task.tile);
    }
  }
}

static void clear_pending_inserts(tl_write_queue* q) {
  for (int32_t i = 0; i < q->inserts_len; i++) {
    image_db_insert* ins = &q->inserts[i];
    free(ins->img.data);
  }

  q->inserts_len = 0;
}

static void* commit_pending(void* arg) {
  tl_write_queue* q = (tl_write_queue*)arg;

  for (;;) {
    tc_sema_wait(&q->sema);

    pthread_mutex_lock(&q->lock);
    int32_t queued = q->write_queue.length;
    if (queued >= q->full_threshold) {
      tl_log("write queue full, begin clear");

      populate_pending_inserts(q, q->full_threshold);

      // Block render threads while an insert is in progress.
      int32_t res = image_db_insert_batch(q->db, q->inserts, q->inserts_len);
      pthread_mutex_unlock(&q->lock);
      clear_pending_inserts(q);

      tl_log("write queue clear done, status = %d", res);
    } else if (queued >= q->insert_threshold) {
      populate_pending_inserts(q, queued);
      pthread_mutex_unlock(&q->lock);
      tl_log("begin batch insert");
      int32_t res = image_db_insert_batch(q->db, q->inserts, q->inserts_len);
      clear_pending_inserts(q);
      tl_log("batch insert = %d", res);
    } else {
      pthread_mutex_unlock(&q->lock);
    }
  }
}

tl_write_queue* tl_write_queue_create(image_db* db) {
  tl_write_queue* q = (tl_write_queue*)calloc(1, sizeof(tl_write_queue));
  q->db = db;
  tc_queue_init(&q->write_queue, sizeof(write_task));
  q->full_threshold = 1024;
  q->insert_threshold = 64;
  q->inserts_len = 0;
  q->inserts =
      (image_db_insert*)calloc(q->full_threshold, sizeof(image_db_insert));
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

void tl_write_queue_destroy(tl_write_queue* q) {
  free(q->inserts);
  free(q);
}
