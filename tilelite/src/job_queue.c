#include "job_queue.h"
#include <stdlib.h>

static int job_queue_full(const job_queue* q) {
  if (q->length == q->capacity) {
    return 1;
  }

  return 0;
}

job_queue* job_queue_create() {
  job_queue* q = (job_queue*)calloc(1, sizeof(job_queue));
  job_queue_init(q);

  return q;
}

void job_queue_init(job_queue* q) {
  q->capacity = 32;
  q->jobs = (job*)calloc(q->capacity, sizeof(job));
}

void job_queue_push(job_queue* q, job j) {

  if (job_queue_full(q)) {
    q->capacity = q->capacity * 1.5;
    q->jobs = (job*)realloc(q->jobs, q->capacity * sizeof(job));
  }

  const int32_t insert_idx = (q->start + q->length) % q->capacity;
  q->jobs[insert_idx] = j;
  q->length++;
}

int job_queue_pop(job_queue* q, job* j) {
  if (q->length > 0) {
    *j = q->jobs[q->start];
    q->start = (q->start + 1) % q->capacity;
    q->length--;
    return 1;
  }

  return 0;
}
