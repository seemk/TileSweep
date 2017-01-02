#include "task_queue.h"
#include <stdlib.h>

static int task_queue_full(const task_queue* q) {
  if (q->length == q->capacity) {
    return 1;
  }

  return 0;
}

task_queue* task_queue_create() {
  task_queue* q = (task_queue*)calloc(1, sizeof(task_queue));
  task_queue_init(q);

  return q;
}

void task_queue_init(task_queue* q) {
  q->capacity = 32;
  q->tasks = (task**)calloc(q->capacity, sizeof(task*));
}

void task_queue_push(task_queue* q, task* t) {

  if (task_queue_full(q)) {
    q->capacity = q->capacity * 1.5;
    q->tasks = (task**)realloc(q->tasks, q->capacity * sizeof(task*));
  }

  const int32_t insert_idx = (q->start + q->length) % q->capacity;
  q->tasks[insert_idx] = t;
  q->length++;
}

int task_queue_pop(task_queue* q, task** t) {
  if (q->length > 0) {
    *t = q->tasks[q->start];
    q->start = (q->start + 1) % q->capacity;
    q->length--;
    return 1;
  }

  return 0;
}
