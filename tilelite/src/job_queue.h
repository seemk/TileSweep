#pragma once

#include <stdint.h>

typedef struct {
  int32_t id;
} job;

typedef struct {
  int32_t start;
  int32_t length;
  int32_t capacity;
  job* jobs;
} job_queue;

job_queue* job_queue_create();
void job_queue_push(job_queue* q, job j);
int job_queue_pop(job_queue* q, job* j);
