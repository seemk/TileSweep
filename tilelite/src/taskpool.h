#pragma once
#include <pthread.h>
#include <stdatomic.h>
#include "task.h"
#include "task_queue.h"

typedef enum { TP_HIGH, TP_MED, TP_LOW, TP_COUNT } task_priority;

typedef struct {
  task_queue queue;
  pthread_mutex_t lock;
} pool_queue;

typedef struct {
  int num_threads;
  void* threads;
  pool_queue* queues[TP_COUNT];
  void* sema;
  atomic_int insert_idx;
} taskpool;

taskpool* taskpool_create(int32_t threads);
void taskpool_wait(taskpool* pool, task* t, task_priority priority);
void taskpool_post(taskpool* pool, task* t, task_priority priority);
void taskpool_destroy(taskpool* pool);
