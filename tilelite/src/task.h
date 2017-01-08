#pragma once

#include <stdint.h>
#include <pthread.h>
#include <semaphore.h>

typedef enum {
  TASK_FIREFORGET,
  TASK_SINGLEWAIT,
  TASK_MULTIWAIT
} task_type;

typedef struct {
  int32_t id;
  task_type type;
  pthread_mutex_t lock;
  pthread_cond_t cv;
  void* (*execute)(void*); 
  void* arg;
  sem_t* waitall_sema; 
} task;

task* task_create(void* (*execute)(void*), void* arg);
void task_destroy(task* t);
