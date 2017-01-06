#pragma once
#include <pthread.h>
#include <stdatomic.h>
#include "task.h"
#include "task_queue.h"

typedef enum { TP_LOW, TP_HIGH } task_priority;

typedef struct {
  task_queue queue;
  pthread_mutex_t lock;
} pool_queue;

typedef struct {
  int num_threads;
  void* threads;
  void* sema;
  pool_queue* low_prio_queues;
  pool_queue* high_prio_queues;
  atomic_int insert_idx;
} taskpool;

taskpool* taskpool_create(int threads);
void taskpool_wait(taskpool* pool, task* t, task_priority priority);
void taskpool_post(taskpool* pool, task* t, task_priority priority);
void taskpool_destroy(taskpool* pool);
