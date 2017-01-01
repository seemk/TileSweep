#pragma once

#include <stdint.h>
#include "task.h"

typedef struct {
  int32_t start;
  int32_t length;
  int32_t capacity;
  task* tasks;
} task_queue;

task_queue* task_queue_create();
void task_queue_init(task_queue* q);
void task_queue_push(task_queue* q, task t);
int task_queue_pop(task_queue* q, task* t);
