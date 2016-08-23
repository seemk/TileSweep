#pragma once

#include <stdint.h>
#include "common.h"
#include "tl_math.h"

enum tl_request_type : uint64_t {
  rq_invalid = 0,
  rq_tile = 1,
  rq_prerender = 2,
  rq_prerender_img = 3,
  rq_server_info = 4,

  RQ_COUNT = 5
};

struct tl_tile {
  int w;
  int h;
  int x;
  int y;
  int z;

  uint64_t hash() const {
    return (uint64_t(z) << 40) | (uint64_t(x) << 20) | uint64_t(y);
  }

  bool valid() const { return w > -1 && h > -1 && x > -1 && y > -1 && z > -1; }
};

struct tl_prerender {
  int num_points;
  vec2d* points;
  int num_zoom_levels;
  uint8_t zoom[MAX_ZOOM_LEVEL + 1];
  int width;
  int height;
};

union tl_request_union {
  tl_tile tile;
  tl_prerender prerender;
};

struct tl_request {
  int64_t request_time;
  int client_fd;
  tl_request_type type;
  tl_request_union as;
};
