#pragma once
#include "tl_tile.h"
#include "image.h"

typedef struct task task;

typedef struct {
  tl_tile tile;
  image img;
  struct tile_renderer* renderer;
  int success;
} render_task;

struct task {
  int32_t id;
  void* (*execute)(task* t); 
  union {
    render_task ren_task;
  } as;
};
