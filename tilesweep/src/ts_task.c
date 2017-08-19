#include "ts_task.h"
#include <stdlib.h>

ts_task* ts_task_create(ts_task_fn execute, void* arg) {
  ts_task* t = (ts_task*)calloc(1, sizeof(ts_task));
  t->execute = execute;
  t->arg = arg;
  return t;
}

void ts_task_destroy(ts_task* t) { free(t); }
