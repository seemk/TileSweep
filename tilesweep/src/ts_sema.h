#pragma once

#include <stdint.h>

#if __APPLE__
#include <mach/mach.h>

typedef struct { semaphore_t sema; } ts_sema;

#elif __unix__
#include <semaphore.h>

typedef struct { sem_t sema; } ts_sema;

#endif

void ts_sema_init(ts_sema* sema, uint32_t value);
void ts_sema_post(ts_sema* sema, uint32_t count);
void ts_sema_wait(ts_sema* sema);
void ts_sema_deinit(ts_sema* sema);
