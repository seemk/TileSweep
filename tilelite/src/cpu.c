#include "cpu.h"
#include <unistd.h>

int32_t cpu_core_count() {
  return sysconf(_SC_NPROCESSORS_ONLN);
}
