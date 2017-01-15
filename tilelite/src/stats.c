#include "stats.h"
#include <stdlib.h>
#include <string.h>
#include "stretchy_buffer.h"
#include "tl_time.h"

prerender_job_stats* prerender_job_stats_create(const vec2d* coordinates,
                                                int32_t count) {
  prerender_job_stats* stats =
      (prerender_job_stats*)calloc(1, sizeof(prerender_job_stats));
  stats->start_time = tl_usec_now();
  atomic_init(&stats->max_tiles, 0);
  atomic_init(&stats->tile_estimate, 0);
  atomic_init(&stats->current_tiles, 0);

  stats->coordinates = (vec2d*)calloc(count, sizeof(vec2d));
  memcpy(stats->coordinates, coordinates, count * sizeof(vec2d));

  return stats;
}

void prerender_job_stats_destroy(prerender_job_stats* stats) {
  free(stats->coordinates);
  free(stats);
}

tilelite_stats* tilelite_stats_create() {
  tilelite_stats* s = (tilelite_stats*)calloc(1, sizeof(tilelite_stats));
  pthread_mutex_init(&s->lock, NULL);
  return s;
}

void tilelite_stats_add_prerender(tilelite_stats* stats,
                                  prerender_job_stats* prerender) {
  pthread_mutex_lock(&stats->lock);
  sb_push(stats->prerenders, prerender);
  pthread_mutex_unlock(&stats->lock);
}

int32_t tilelite_stats_remove_prerender(tilelite_stats* stats,
                                        prerender_job_stats* prerender) {
  pthread_mutex_lock(&stats->lock);

  int32_t removed = 0;
  if (stats->prerenders) {
    const int32_t capacity = stb__sbm(stats->prerenders);
    const int32_t length = sb_count(stats->prerenders);
    int32_t rem_idx = -1;

    for (int32_t i = 0; i < length; i++) {
      if (prerender == stats->prerenders[i]) {
        rem_idx = i;
        break;
      }
    }

    if (rem_idx > -1) {
      if (length > 1) {
        int32_t last = length - 1;
        stats->prerenders[rem_idx] = stats->prerenders[last];
      }
      stb__sbn(stats->prerenders)--;
    }
  }

  pthread_mutex_unlock(&stats->lock);
  return removed;
}
