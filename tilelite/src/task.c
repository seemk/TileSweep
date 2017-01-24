#include "task.h"
#include <assert.h>
#include <stdlib.h>

task* task_create(task_fn execute, void* arg) {
  task* t = (task*)calloc(1, sizeof(task));
  t->execute = execute;
  t->arg = arg;
  return t;
}

void task_destroy(task* t) {
  free(t);
}
