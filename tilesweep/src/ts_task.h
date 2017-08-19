#pragma once

#include <pthread.h>
#include <stdint.h>
#include "ts_sema.h"

typedef enum { TASK_FIREFORGET, TASK_WAIT } ts_task_type;

typedef struct { int32_t executing_thread_idx; } ts_task_extra_info;

typedef void* (*ts_task_fn)(void*, const ts_task_extra_info*);

typedef struct {
  ts_task_type type;
  ts_task_fn execute;
  void* arg;
  ts_sema* waitall_sema;
} ts_task;

ts_task* ts_task_create(ts_task_fn execute, void* arg);
void ts_task_destroy(ts_task* t);
