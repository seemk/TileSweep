#include "minunit.h"
#include <stdlib.h>
#include <stdio.h>
#include "../job_queue.h"

static char* job_queue_test() {
  job_queue* q = job_queue_create();
 
  job j;
  mu_assert("empty pop", !job_queue_pop(q, &j));

  int num_jobs = q->capacity + 100;
  for (int i = 0; i < num_jobs; i++) {
    job j1 = {.id = i};
    job_queue_push(q, j1);
  }

  for (int i = 0; i < num_jobs; i++) {
    int success = job_queue_pop(q, &j);
    mu_assert("pop", success && j.id == i);
  }

  mu_assert("pop after emptying", !job_queue_pop(q, &j));

  return NULL;
}

static char* all() {
  mu_run_test(job_queue_test);
  return NULL;
}

int main(int argc, char** argv) {
  char* result = all();
  if (result != NULL) {
    printf("%s\n", result);
  } else {
    printf("ok\n");
  }

  return result != NULL;
}
