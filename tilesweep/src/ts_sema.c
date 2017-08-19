#include "ts_sema.h"

#if __APPLE__

void ts_sema_init(ts_sema* sema, uint32_t value) {
  semaphore_create(mach_task_self(), &sema->sema, SYNC_POLICY_FIFO, value);
}

void ts_sema_post(ts_sema* sema, uint32_t count) {
  for (uint32_t i = 0; i < count; i++) {
    semaphore_signal(sema->sema);
  }
}

void ts_sema_wait(ts_sema* sema) { semaphore_wait(sema->sema); }

void ts_sema_deinit(ts_sema* sema) {
  semaphore_destroy(mach_task_self(), sema->sema);
}

#elif __unix__

void ts_sema_init(ts_sema* sema, uint32_t value) {
  sem_init(&sema->sema, 0, value);
}

void ts_sema_post(ts_sema* sema, uint32_t count) {
  for (uint32_t i = 0; i < count; i++) {
    sem_post(&sema->sema);
  }
}

void ts_sema_wait(ts_sema* sema) { sem_wait(&sema->sema); }

void ts_sema_deinit(ts_sema* sema) { sem_destroy(&sema->sema); }

#endif
