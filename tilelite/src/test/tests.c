#include <stdio.h>
#include <stdlib.h>
#include "../poly_hit_test.h"
#include "../task_queue.h"
#include "minunit.h"

static char* polygon_hit_test() {
  vec2i points[] = {{4, 3}, {3, 4}, {3, 6}, {4, 7},
                    {6, 7}, {7, 6}, {7, 4}, {6, 3}};

  poly_hit_test t;
  poly_hit_test_init(&t, points, 8);

  mu_assert("out (3, 3)", !poly_hit_test_check(&t, (vec2i){.x = 3, .y = 3}));
  mu_assert("out (4, 3)", !poly_hit_test_check(&t, (vec2i){.x = 4, .y = 3}));
  mu_assert("in (5, 5)", poly_hit_test_check(&t, (vec2i){.x = 5, .y = 5}));
  mu_assert("in (6, 6)", poly_hit_test_check(&t, (vec2i){.x = 6, .y = 6}));

  return NULL;
};

static char* task_queue_test() {
  task_queue* q = task_queue_create();

  task* t;
  mu_assert("empty pop", !task_queue_pop(q, &t));

  int num_tasks = q->capacity + 100;
  for (int i = 0; i < num_tasks; i++) {
    task* j1 = task_create(NULL, NULL);
    j1->id = i;
    task_queue_push(q, j1);
  }

  for (int i = 0; i < num_tasks; i++) {
    int success = task_queue_pop(q, &t);
    mu_assert("pop", success && t->id == i);
    task_destroy(t);
  }

  mu_assert("pop after emptying", !task_queue_pop(q, &t));

  return NULL;
}

static char* all() {
  mu_run_test(task_queue_test);
  mu_run_test(polygon_hit_test);
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
