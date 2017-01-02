#include "taskpool.h"
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include "tl_log.h"

typedef struct {
  int thread_num;
  taskpool* pool;
  pthread_t id;
  sem_t* sema;
} pool_thread_info;

int pool_queue_pop(pool_queue* q, task** t) {
  pthread_mutex_lock(&q->lock);
  int res = task_queue_pop(&q->queue, t);
  pthread_mutex_unlock(&q->lock);
  return res;
}

void pool_queue_push(pool_queue* q, task* t) {
  pthread_mutex_lock(&q->lock);
  task_queue_push(&q->queue, t);
  pthread_mutex_unlock(&q->lock);
}

static void* pool_task(void* arg) {
  pool_thread_info* info = (pool_thread_info*)arg;

  for (;;) {
    task* t = NULL;
    for (int i = 0; i < info->pool->num_threads; i++) {
      if (pool_queue_pop(&info->pool->high_prio_queues[i], &t)) {
        break;
      }
    }

    if (!t) {
      for (int i = 0; i < info->pool->num_threads; i++) {
        if (pool_queue_pop(&info->pool->low_prio_queues[i], &t)) {
          break;
        }
      }
    }

    if (t) {
#ifdef DEBUG
      int64_t start_time = tl_usec_now();
#endif
      pthread_mutex_lock(&t->lock);
      t->execute(t->arg);
      pthread_cond_signal(&t->cv);
      pthread_mutex_unlock(&t->lock);
#ifdef DEBUG
      tl_log("[pool-%d] task exec: %.3f ms", info->thread_num,
             (tl_usec_now() - start_time) / 1000.0);
#endif
    }

    sem_wait(info->sema);
  }
}

taskpool* taskpool_create(int threads) {
  taskpool* pool = (taskpool*)calloc(1, sizeof(taskpool));
  pool->num_threads = threads;
  pool->threads = calloc(threads, sizeof(pool_thread_info));
  pool->sema = calloc(1, sizeof(sem_t));
  pool->low_prio_queues = (pool_queue*)calloc(threads, sizeof(pool_queue));
  pool->high_prio_queues = (pool_queue*)calloc(threads, sizeof(pool_queue));

  atomic_init(&pool->insert_idx, 0);

  sem_init(pool->sema, 0, 0);

  pool_thread_info* pool_threads = (pool_thread_info*)pool->threads;
  for (int i = 0; i < threads; i++) {
    task_queue_init(&pool->low_prio_queues[i].queue);
    pthread_mutex_init(&pool->low_prio_queues[i].lock, NULL);

    task_queue_init(&pool->high_prio_queues[i].queue);
    pthread_mutex_init(&pool->high_prio_queues[i].lock, NULL);

    pool_thread_info* t = &pool_threads[i];
    t->thread_num = i;
    t->pool = pool;
    t->sema = pool->sema;
    pthread_create(&t->id, NULL, pool_task, t);
  }

  return pool;
}

void taskpool_do(taskpool* pool, task* t, task_priority priority) {
  int queue_idx = atomic_fetch_add(&pool->insert_idx, 1) % pool->num_threads;
  pool_queue* q = priority == TP_HIGH ? &pool->high_prio_queues[queue_idx]
                                      : &pool->low_prio_queues[queue_idx];
  pool_queue_push(q, t);
  pthread_mutex_lock(&t->lock);
  sem_post(pool->sema);
  pthread_cond_wait(&t->cv, &t->lock);
}

void taskpool_destroy(taskpool* pool) {
  pool_thread_info* pool_threads = (pool_thread_info*)pool->threads;

  for (int i = 0; i < pool->num_threads; i++) {
    pthread_join(pool_threads[i].id, NULL);
  }

  free(pool->threads);
  free(pool->sema);
  free(pool);
}
