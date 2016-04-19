#include "miniunit.h"
#include "../tl_message.h"
#include <string.h>
#include <stdio.h>

const char* empty_string() {
  const char* empty = "";
  tl_message m;
  mu_assert("empty message", tlm_start(empty, strlen(empty), &m) == 0);
  return NULL;
}

const char* short_string() {
  const char* short_msg = "b\0";
  tl_message m;
  mu_assert("short message", tlm_start(short_msg, 2, &m) == 0);
  return NULL;
}

static const char* tests() {
  mu_run_test(empty_string);
  return NULL;
}

int main(int argc, char** argv) {
  const char* res = tests();
  if (res != 0) {
    printf("failed: %s\n", res);
  }
  return res != 0;
}
