#include "prerender.h"
#include <assert.h>
#include <limits.h>
#include <math.h>
#include <string.h>
#include "poly_hit_test.h"
#include "stretchy_buffer.h"

collision_check_job** make_collision_check_jobs(
    const vec2d* coordinates, int32_t num_coordinates, int32_t min_zoom,
    int32_t max_zoom, int32_t tile_size, int32_t job_check_limit) {
  assert(num_coordinates > 2);

  if (num_coordinates < 3) {
    return NULL;
  }

  collision_check_job** jobs = NULL;
  for (int32_t z = min_zoom; z <= max_zoom; z++) {
    vec2i* tile_coords = NULL;

    for (int32_t i = 0; i < num_coordinates; i++) {
      vec2d mercator_pt = coordinates[i];
      vec2i tile = mercator_to_tile(mercator_pt.x, mercator_pt.y, z, tile_size);

      int already_exists = 0;
      for (int32_t j = 0; j < sb_count(tile_coords); j++) {
        vec2i existing = tile_coords[j];
        if (tile.x == existing.x && tile.y == existing.y) {
          already_exists = 1;
          break;
        }
      }

      if (!already_exists) {
        sb_push(tile_coords, tile);
      }
    }

    vec2i top_left = {.x = INT32_MAX, .y = INT32_MAX};
    vec2i bot_right = {.x = INT32_MIN, .y = INT32_MIN};

    for (int32_t i = 0; i < sb_count(tile_coords); i++) {
      vec2i pt = tile_coords[i];

      if (pt.x < top_left.x) top_left.x = pt.x;
      if (pt.y < top_left.y) top_left.y = pt.y;
      if (pt.x > bot_right.x) bot_right.x = pt.x;
      if (pt.y > bot_right.y) bot_right.y = pt.y;
    }

    assert(top_left.x != INT32_MAX && top_left.y != INT32_MAX &&
           bot_right.x != INT32_MIN && bot_right.y != INT32_MIN);

    int32_t w = bot_right.x - top_left.x;
    int32_t h = bot_right.y - top_left.y;

    if (w <= 0) w = 1;
    if (h <= 0) h = 1;

    const int32_t num_jobs =
        (int32_t)ceil((double)(w * h) / (double)job_check_limit);

    assert(num_jobs > 0);

    const int32_t num_poly_tiles = sb_count(tile_coords);
    for (int32_t i = 0; i < num_jobs - 1; i++) {
      int32_t start_index = job_check_limit * i;
      int32_t end_index = job_check_limit * (i + 1) - 1;
      int32_t tx = end_index % w;
      int32_t ty = end_index / w;
      collision_check_job* job =
          (collision_check_job*)calloc(1, sizeof(collision_check_job));
      job->top_left = top_left;
      job->bot_right = bot_right;
      job->tile_size = tile_size;
      job->x_start = top_left.x + (start_index % w);
      job->y_start = top_left.y + (start_index / w);
      job->x_end = top_left.x + (end_index % w);
      job->y_end = top_left.y + (end_index / w);
      job->zoom = z;
      job->num_tile_coordinates = num_poly_tiles;
      job->tile_coordinates = (vec2i*)calloc(num_poly_tiles, sizeof(vec2i));

      memcpy(job->tile_coordinates, tile_coords,
             num_poly_tiles * sizeof(vec2i));

      sb_push(jobs, job);
    }

    const int32_t last_job_idx = job_check_limit * (num_jobs - 1);
    collision_check_job* last =
        (collision_check_job*)calloc(1, sizeof(collision_check_job));
    last->top_left = top_left;
    last->bot_right = bot_right;
    last->tile_size = tile_size;
    last->x_start = top_left.x + last_job_idx % w;
    last->y_start = top_left.y + last_job_idx / w;
    last->x_end = bot_right.x;
    last->y_end = bot_right.y;
    last->zoom = z;
    last->num_tile_coordinates = num_poly_tiles;
    last->tile_coordinates = (vec2i*)calloc(num_poly_tiles, sizeof(vec2i));

    memcpy(last->tile_coordinates, tile_coords, num_poly_tiles * sizeof(vec2i));

    sb_push(jobs, last);
    sb_free(tile_coords);
  }

  return jobs;
}

vec2i* calc_tiles(const collision_check_job* job) {
  poly_hit_test test_ctx;
  poly_hit_test_init(&test_ctx, job->tile_coordinates,
                     job->num_tile_coordinates);

  vec2i* tiles = NULL;
  for (int32_t y = job->y_start; y <= job->y_end; y++) {
    for (int32_t x = job->x_start; x <= job->x_end; x++) {
      vec2i pt = {.x = x, .y = y};

      int inside =
          poly_hit_test_check(&test_ctx, pt) ||
          poly_hit_test_check(&test_ctx, (vec2i){.x = x + 1, .y = y}) ||
          poly_hit_test_check(&test_ctx, (vec2i){.x = x, .y = y + 1}) ||
          poly_hit_test_check(&test_ctx, (vec2i){.x = x + 1, .y = y + 1});

      if (inside) {
        sb_push(tiles, pt);
      }
    }
  }

  poly_hit_test_destroy(&test_ctx);

  return tiles;
}
