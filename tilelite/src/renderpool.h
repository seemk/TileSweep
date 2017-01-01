#pragma once
#include "job_queue.h"
#include <pthread.h>
#include <stdatomic.h>

typedef struct {
  job_queue queue;
  pthread_mutex_t lock;
} pool_queue;

typedef struct {
  int num_threads;
  void* threads;
  void* sema;
  pool_queue* low_prio_queues;
  pool_queue* high_prio_queues;
  atomic_int insert_idx;
} renderpool;

renderpool* renderpool_create(int threads);
void renderpool_do(renderpool* pool, int task);
void renderpool_destroy(renderpool* pool);
