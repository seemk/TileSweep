#include "prerender.h"
#include <assert.h>
#include <limits.h>
#include <math.h>
#include <string.h>
#include "stretchy_buffer.h"

static int32_t exists(const vec2d* points, int32_t len, vec2d p) {
  for (int32_t i = 0; i < len; i++) {
    vec2d e = points[i];
    if (p.x == e.x && p.y == e.y) {
      return 1;
    }
  }

  return 0;
}

tile_calc_job** make_tile_calc_jobs(const vec2d* coordinates,
                                    int32_t num_coordinates, int32_t min_zoom,
                                    int32_t max_zoom, int32_t tile_size,
                                    int32_t tiles_per_job) {
  if (num_coordinates < 3) {
    return NULL;
  }

  tile_calc_job** jobs = NULL;
  for (int32_t z = min_zoom; z <= max_zoom; z++) {
    vec2d* tile_polygon = NULL;

    for (int32_t i = 0; i < num_coordinates; i++) {
      vec2d mercator_pt = coordinates[i];
      vec2d tile = mercator_to_tile(mercator_pt.x, mercator_pt.y, z, tile_size);
      tile.x += 0.5;
      tile.y += 0.5;

      if (!exists(tile_polygon, sb_count(tile_polygon), tile)) {
        sb_push(tile_polygon, tile);
      }
    }

    tile_calc_job* job = (tile_calc_job*)calloc(1, sizeof(tile_calc_job));
    job->tile_size = tile_size;
    job->zoom = z;
    job->fill_limit = tiles_per_job;

    fill_poly_state_init(&job->fill_state, tile_polygon,
                         sb_count(tile_polygon));

    sb_push(jobs, job);
    sb_free(tile_polygon);
  }

  return jobs;
}

vec2i* calc_tiles(tile_calc_job* job, int32_t* count) {
  vec2d* centered_tiles = fill_poly_advance(&job->fill_state, job->fill_limit);

  const int32_t num_tiles = sb_count(centered_tiles);
  *count = num_tiles;

  if (num_tiles <= 0) {
    sb_free(centered_tiles);
    return NULL;
  }

  vec2i* tiles = (vec2i*)calloc(num_tiles, sizeof(vec2i));

  for (int32_t i = 0; i < num_tiles; i++) {
    double x = centered_tiles[i].x - 0.5;
    double y = centered_tiles[i].y - 0.5;

    tiles[i].x = (int32_t)x;
    tiles[i].y = (int32_t)y;
  }

  return tiles;
}
