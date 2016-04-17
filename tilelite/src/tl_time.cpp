#include "tl_time.h"
#include <sys/time.h>

int64_t tl_usec_now() {
  struct timeval t;
  gettimeofday(&t, 0);
  return t.tv_sec * 1000000 + t.tv_usec;
}
