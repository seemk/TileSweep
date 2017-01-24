#include "../tc_queue.h"
#include "minunit.h"

static const char* test_queue() {
  tc_queue* q = tc_queue_create(sizeof(int64_t));

  int64_t t;
  mu_assert("empty pop", !tc_queue_pop(q, &t));

  int32_t num_tasks = 120;
  for (int64_t i = 0; i < num_tasks; i++) {
    tc_queue_push(q, &i);
  }

  for (int64_t i = 0; i < num_tasks; i++) {
    int32_t success = tc_queue_pop(q, &t);
    mu_assert("pop", success && t == i);
  }

  mu_assert("pop after emptying", !tc_queue_pop(q, &t));

  return NULL;
}
