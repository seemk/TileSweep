#pragma once
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

int64_t usec_now();
uint32_t seconds_now();

#ifdef __cplusplus
}
#endif

int32_t cpu_core_count();

#define ts_log(fmt, ...)                                                       \
  do {                                                                         \
    double __log_now_s = usec_now() / 1000000.0;                               \
    time_t __log_t = __log_now_s;                                              \
    struct tm* __log_now = localtime(&__log_t);                                \
    char __log_buf[32];                                                        \
    strftime(__log_buf, 32, "%Y-%m-%d %H:%M:%S", __log_now);                   \
    printf("[%s:%03d] " fmt "\n", __log_buf,                                   \
           (int)((__log_now_s - floor(__log_now_s)) * 1000.0), ##__VA_ARGS__); \
  } while (0)
