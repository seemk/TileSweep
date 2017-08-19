#include "ts_write_queue.h"
#include <pthread.h>
#include <stdlib.h>
#include "platform.h"
#include "ts_queue.h"
#include "ts_sema.h"

struct ts_write_queue {
  ts_queue write_queue;
  int32_t full_threshold;
  int32_t insert_threshold;
  int32_t inserts_len;
  int32_t force_commit;
  image_db_insert* inserts;
  pthread_t write_thread;
  pthread_mutex_t lock;
  ts_sema sema;
  image_db* db;
};

static void populate_pending_inserts(ts_write_queue* q, int32_t num) {
  for (int32_t i = 0; i < num; i++) {
    write_task task;
    if (ts_queue_pop(&q->write_queue, &task)) {
      image_db_insert* ins = &q->inserts[q->inserts_len++];
      ins->img = task.img;
      ins->image_hash = task.image_hash;
      ins->position_hash = tile_hash(&task.tile);
    }
  }
}

static void clear_pending_inserts(ts_write_queue* q) {
  for (int32_t i = 0; i < q->inserts_len; i++) {
    image_db_insert* ins = &q->inserts[i];
    free(ins->img.data);
  }

  q->inserts_len = 0;
}

static void* commit_pending(void* arg) {
  ts_write_queue* q = (ts_write_queue*)arg;

  for (;;) {
    ts_sema_wait(&q->sema);

    pthread_mutex_lock(&q->lock);
    int32_t queued = q->write_queue.length;
    if (queued >= q->full_threshold) {
      ts_log("write queue full, begin clear");

      populate_pending_inserts(q, q->full_threshold);

      // Block render threads while an insert is in progress.
      int32_t res = image_db_insert_batch(q->db, q->inserts, q->inserts_len);
      pthread_mutex_unlock(&q->lock);
      clear_pending_inserts(q);

      ts_log("write queue clear done, status = %d", res);
    } else if (queued >= q->insert_threshold || q->force_commit) {
      populate_pending_inserts(q, queued);
      q->force_commit = 0;
      pthread_mutex_unlock(&q->lock);
      ts_log("begin batch insert");
      int32_t res = image_db_insert_batch(q->db, q->inserts, q->inserts_len);
      clear_pending_inserts(q);
      ts_log("batch insert = %d", res);
    } else {
      pthread_mutex_unlock(&q->lock);
    }
  }
}

ts_write_queue* ts_write_queue_create(image_db* db) {
  ts_write_queue* q = (ts_write_queue*)calloc(1, sizeof(ts_write_queue));
  q->db = db;
  ts_queue_init(&q->write_queue, sizeof(write_task));
  q->full_threshold = 1024;
  q->insert_threshold = 64;
  q->inserts_len = 0;
  q->inserts =
      (image_db_insert*)calloc(q->full_threshold, sizeof(image_db_insert));
  pthread_mutex_init(&q->lock, NULL);
  ts_sema_init(&q->sema, 0);
  pthread_create(&q->write_thread, NULL, commit_pending, q);
  return q;
}

void ts_write_queue_push(ts_write_queue* q, ts_tile tile, image img,
                         uint64_t image_hash) {
  write_task task = {.tile = tile,
                     .img = img,
                     .image_hash = image_hash,
                     .start_time = usec_now()};

  pthread_mutex_lock(&q->lock);
  ts_queue_push(&q->write_queue, &task);
  pthread_mutex_unlock(&q->lock);
  ts_sema_post(&q->sema, 1);
}

void ts_write_queue_destroy(ts_write_queue* q) {
  free(q->inserts);
  free(q);
}

void ts_write_queue_commit(ts_write_queue* q) {
  pthread_mutex_lock(&q->lock);
  q->force_commit = 1;
  pthread_mutex_unlock(&q->lock);
  ts_sema_post(&q->sema, 1);
}
