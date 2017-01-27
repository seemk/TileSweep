#pragma once

#include <stdint.h>

#if __APPLE__
#include <mach/mach.h>

typedef struct { semaphore_t sema; } tc_sema;

#elif __unix__
#include <semaphore.h>

typedef struct { sem_t sema; } tc_sema;

#endif

void tc_sema_init(tc_sema* sema, uint32_t value);
void tc_sema_post(tc_sema* sema, uint32_t count);
void tc_sema_wait(tc_sema* sema);
void tc_sema_deinit(tc_sema* sema);
