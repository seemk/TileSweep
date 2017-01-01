#include "taskpool.h"
#include <semaphore.h>
#include <stdlib.h>
#include <stdio.h>
#include "tl_log.h"

typedef struct {
  int thread_num;
  taskpool* pool; 
  pthread_t id;
  sem_t* sema;
} pool_thread_info;

int pool_queue_pop(pool_queue* q, task* t) {
  pthread_mutex_lock(&q->lock);
  int res = task_queue_pop(&q->queue, t);
  pthread_mutex_unlock(&q->lock);
  return res;
}

int32_t pool_queue_push(pool_queue* q, task t) {
  pthread_mutex_lock(&q->lock);
  task_queue_push(&q->queue, t);
  q->id_counter++;
  int32_t id = q->id_counter;
  pthread_mutex_unlock(&q->lock);
  return id;
}

static void* pool_task(void* arg) {
  pool_thread_info* info = (pool_thread_info*)arg;

  for (;;) {

    int fetched = 0;
    task t;
    for (int i = 0; i < info->pool->num_threads; i++) {
      if (pool_queue_pop(&info->pool->high_prio_queues[i], &t)) {
        fetched = 1;
        break;
      }
    }

    if (!fetched) {
      for (int i = 0; i < info->pool->num_threads; i++) {
        if (pool_queue_pop(&info->pool->low_prio_queues[i], &t)) {
          fetched = 1;
          break;
        }
      }
    }

    if (fetched) {
      t.execute(&t);
    }

    sem_wait(info->sema);
  }

  tl_log("pool thread %d exiting", info->thread_num);
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

int32_t taskpool_do(taskpool* pool, task t) {
  int queue_idx = atomic_fetch_add(&pool->insert_idx, 1) % pool->num_threads;
  int32_t id = pool_queue_push(&pool->high_prio_queues[queue_idx], t);
  sem_post(pool->sema);
  return id;
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
