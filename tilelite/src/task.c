#include "task.h"
#include <stdlib.h>

task* task_create(void* (*execute)(void*), void* arg) {
  task* t = (task*)calloc(1, sizeof(task));
  pthread_mutex_init(&t->lock, NULL);
  pthread_cond_init(&t->cv, NULL);
  t->execute = execute;
  t->arg = arg;

  return t;
}

void task_destroy(task* t) {
  pthread_mutex_destroy(&t->lock);
  pthread_cond_destroy(&t->cv);
  free(t);
}
