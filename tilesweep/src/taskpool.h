#pragma once
#include <pthread.h>
#include <stdatomic.h>
#include "ts_queue.h"
#include "ts_sema.h"
#include "ts_task.h"

typedef enum { TP_HIGH, TP_MED, TP_LOW, TP_COUNT } task_priority;

typedef struct {
  ts_queue queue;
  pthread_mutex_t lock;
} pool_queue;

typedef struct {
  int num_threads;
  void* threads;
  pool_queue* queues[TP_COUNT];
  ts_sema sema;
  atomic_int insert_idx;
} taskpool;

taskpool* taskpool_create(int32_t threads);
void taskpool_wait(taskpool* pool, ts_task* t, task_priority priority);
void taskpool_post(taskpool* pool, ts_task* t, task_priority priority);
void taskpool_wait_all(taskpool* pool, ts_task** tasks, int32_t count,
                       task_priority priority);
void taskpool_destroy(taskpool* pool);
