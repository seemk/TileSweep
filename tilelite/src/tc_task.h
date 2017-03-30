#pragma once

#include <pthread.h>
#include <stdint.h>
#include "tc_sema.h"

typedef enum { TASK_FIREFORGET, TASK_WAIT } tc_task_type;

typedef struct { int32_t executing_thread_idx; } tc_task_extra_info;

typedef void* (*tc_task_fn)(void*, const tc_task_extra_info*);

typedef struct {
  tc_task_type type;
  tc_task_fn execute;
  void* arg;
  tc_sema* waitall_sema;
} tc_task;

tc_task* tc_task_create(tc_task_fn execute, void* arg);
void tc_task_destroy(tc_task* t);
