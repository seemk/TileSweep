#pragma once

#include <stdint.h>
#include <pthread.h>

typedef struct {
  int32_t id;
  pthread_mutex_t lock;
  pthread_cond_t cv;
  void* (*execute)(void*); 
  void* arg;
  void (*cleanup)(void*);
} task;

task* task_create(void* (*execute)(void*), void* arg);
void task_destroy(task* t);
void task_default_cleanup(void* arg);
