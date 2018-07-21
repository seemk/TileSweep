#include "platform.h"
#include <sys/time.h>
#include <unistd.h>

int64_t usec_now() {
  struct timeval t;
  gettimeofday(&t, 0);
  return t.tv_sec * 1000000 + t.tv_usec;
}

uint32_t seconds_now() {
  return usec_now() / UINT64_C(1000000);
}

int32_t cpu_core_count() { return sysconf(_SC_NPROCESSORS_ONLN); }
