#pragma once
#include "task_queue.h"
#include <pthread.h>
#include <stdatomic.h>
#include "task.h"

typedef struct {
  task_queue queue;
  pthread_mutex_t lock;
  int32_t id_counter;
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
int32_t taskpool_do(taskpool* pool, task t);
void taskpool_destroy(taskpool* pool);
