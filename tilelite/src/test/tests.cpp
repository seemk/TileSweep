#include "miniunit.h"
#include <stdio.h>

static const char* tests() {
  return nullptr;
}

int main(int argc, char** argv) {
  const char* res = tests();

  if (res != nullptr) {
    printf("failed: %s\n", res);
  }

  return res != nullptr;
}
