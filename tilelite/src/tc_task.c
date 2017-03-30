#include "tc_task.h"
#include <stdlib.h>

tc_task* tc_task_create(tc_task_fn execute, void* arg) {
  tc_task* t = (tc_task*)calloc(1, sizeof(tc_task));
  t->execute = execute;
  t->arg = arg;
  return t;
}

void tc_task_destroy(tc_task* t) { free(t); }
