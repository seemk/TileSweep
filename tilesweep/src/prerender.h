#pragma once

#include <stddef.h>
#include "poly_fill.h"
#include "stats.h"
#include "ts_math.h"

typedef struct {
  int32_t id;
  int32_t tile_size;
  int32_t zoom;
  int32_t fill_limit;
  uint64_t estimated_tiles;
  fill_poly_state fill_state;
  prerender_job_stats* stats;
  void* user;
} tile_calc_job;

typedef struct {
  int64_t id;
  int32_t min_zoom;
  int32_t max_zoom;
  vec2d* coordinates;
  size_t num_coordinates;
  int32_t tile_size;
  void* user;
} prerender_req;

tile_calc_job** make_tile_calc_jobs(const vec2d* coordinates,
                                    int32_t num_coordinates, int32_t min_zoom,
                                    int32_t max_zoom, int32_t tile_size,
                                    int32_t tiles_per_job);
vec2i* calc_tiles(tile_calc_job* job, int32_t* count);
