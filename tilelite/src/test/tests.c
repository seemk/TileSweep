#include <stdio.h>
#include <stdlib.h>
#include "minunit.h"
#include "test_poly_fill.c"
#include "test_math.c"
#include "test_queue.c"

static const char* all() {
  mu_run_test(test_poly_fill);
  mu_run_test(test_math);
  mu_run_test(test_queue);
  return NULL;
}

int main(int argc, char** argv) {
  const char* result = all();
  if (result != NULL) {
    printf("%s\n", result);
  } else {
    printf("ok\n");
  }

  return result != NULL;
}
