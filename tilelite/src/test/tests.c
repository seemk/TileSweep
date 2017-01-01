#include "minunit.h"
#include <stdlib.h>
#include <stdio.h>
#include "../task_queue.h"

static char* task_queue_test() {
  task_queue* q = task_queue_create();
 
  task t;
  mu_assert("empty pop", !task_queue_pop(q, &t));

  int num_tasks = q->capacity + 100;
  for (int i = 0; i < num_tasks; i++) {
    task j1 = {.id = i};
    task_queue_push(q, j1);
  }

  for (int i = 0; i < num_tasks; i++) {
    int success = task_queue_pop(q, &t);
    mu_assert("pop", success && t.id == i);
  }

  mu_assert("pop after emptying", !task_queue_pop(q, &t));

  return NULL;
}

static char* all() {
  mu_run_test(task_queue_test);
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
