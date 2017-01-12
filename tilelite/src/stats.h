#pragma once

#include <stdatomic.h>
#include <pthread.h>
#include <stdint.h>
#include "tl_math.h"

typedef struct {
  int64_t start_time;
  atomic_ulong max_tiles;
  atomic_ulong tile_estimate;
  atomic_ulong current_tiles;
  vec2d* coordinates;
  int32_t num_coordinates;
} prerender_job_stats;

typedef struct {
  pthread_mutex_t lock;
  prerender_job_stats** prerenders;
} tilelite_stats;

prerender_job_stats* prerender_job_stats_create(const vec2d* coordinates,
                                                int32_t count);

void prerender_job_stats_destroy(prerender_job_stats* stats);

tilelite_stats* tilelite_stats_create();

void tilelite_stats_add_prerender(tilelite_stats* stats,
                                  prerender_job_stats* prerender);

int32_t tilelite_stats_remove_prerender(tilelite_stats* stats,
                                        prerender_job_stats* prerender);
