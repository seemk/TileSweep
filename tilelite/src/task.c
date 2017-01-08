#include "task.h"
#include <assert.h>
#include <stdlib.h>

task* task_create(task_fn execute, void* arg) {
  task* t = (task*)calloc(1, sizeof(task));
  assert(pthread_mutex_init(&t->lock, NULL) == 0);
  assert(pthread_cond_init(&t->cv, NULL) == 0);
  t->execute = execute;
  t->arg = arg;

  return t;
}

void task_destroy(task* t) {
  pthread_mutex_destroy(&t->lock);
  pthread_cond_destroy(&t->cv);
  free(t);
}
