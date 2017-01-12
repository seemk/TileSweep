#pragma once

#include <stddef.h>
#include "tl_math.h"
#include "stats.h"

typedef struct {
  vec2i top_left;
  vec2i bot_right;
  int32_t id;
  int32_t tile_size;
  int32_t x_start;
  int32_t y_start;
  int32_t x_end;
  int32_t y_end;
  int32_t zoom;
  int32_t num_tile_coordinates;
  vec2i* tile_coordinates;
  void* user;
  prerender_job_stats* stats;
} collision_check_job;

typedef struct {
  int64_t id;
  int32_t min_zoom;
  int32_t max_zoom;
  vec2d* coordinates;
  size_t num_coordinates;
  int32_t tile_size;
  void* user;
} prerender_req;

collision_check_job** make_collision_check_jobs(
    const vec2d* coordinates, int32_t num_coordinates, int32_t min_zoom,
    int32_t max_zoom, int32_t tile_size, int32_t job_check_limit);
vec2i* calc_tiles(const collision_check_job* job);
