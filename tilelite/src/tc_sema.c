#include "tc_sema.h"

#if __APPLE__

void tc_sema_init(tc_sema* sema, uint32_t value) {
  semaphore_create(mach_task_self(), &sema->sema, SYNC_POLICY_FIFO, value);
}

void tc_sema_post(tc_sema* sema, uint32_t count) {
  for (uint32_t i = 0; i < count; i++) {
    semaphore_signal(sema->sema);
  }
}

void tc_sema_wait(tc_sema* sema) { semaphore_wait(sema->sema); }

void tc_sema_deinit(tc_sema* sema) {
  semaphore_destroy(mach_task_self(), sema->sema);
}

#elif __unix__

void tc_sema_init(tc_sema* sema, uint32_t value) {
  sem_init(&sema->sema, 0, value);
}

void tc_sema_post(tc_sema* sema, uint32_t count) {
  for (uint32_t i = 0; i < count; i++) {
    sem_post(&sema->sema);
  }
}

void tc_sema_wait(tc_sema* sema) { sem_wait(&sema->sema); }

void tc_sema_deinit(tc_sema* sema) { sem_destroy(&sema->sema); }

#endif
