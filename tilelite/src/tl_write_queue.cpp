#include "tl_write_queue.h"
#include "concurrentqueue.h"
#include "sema.h"
#include "tl_log.h"
#include "tl_time.h"

template <typename T>
using tl_queue = moodycamel::ConcurrentQueue<T>;

typedef tl_queue<write_task> tl_tile_queue;

struct tl_write_queue {
  tl_write_queue(image_db* db) : db(db) {}
  pthread_t write_thread;
  LightweightSemaphore sema;
  tl_tile_queue write_queue;
  image_db* db;
};

static void* commit_pending(void* arg) {
  tl_write_queue* q = (tl_write_queue*)arg;

  for (;;) {
    q->sema.wait();

    write_task task;
    while (q->write_queue.try_dequeue(task)) {
      if (image_db_add_image(q->db, &task.img, task.image_hash)) {
        image_db_add_position(q->db, tile_hash(&task.tile), task.image_hash);
        int64_t end_time = tl_usec_now();
        tl_tile t = task.tile;
        tl_log("[%d, %d, %d, %d, %d] insert [%.2f ms]", t.w, t.h, t.x, t.y, t.z,
               double(end_time - task.start_time) / 1000.0);
      }

      free(task.img.data);
    }
  }
}

tl_write_queue* tl_write_queue_create(image_db* db) {
  tl_write_queue* q = new tl_write_queue(db);
  pthread_create(&q->write_thread, NULL, commit_pending, q);
  return q;
}

void tl_write_queue_push(tl_write_queue* q, tl_tile tile, image img,
                         uint64_t image_hash) {
  write_task task;
  task.tile = tile;
  task.img = img;
  task.image_hash = image_hash;
  task.start_time = tl_usec_now();

  q->write_queue.enqueue(task);
  q->sema.signal(1);
}

void tl_write_queue_destroy(tl_write_queue* q) { delete q; }
