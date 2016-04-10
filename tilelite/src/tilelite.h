#pragma once

#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include "thread/sema.h"
#include "image.h"
#include "tile.h"
#include "tile_renderer.h"
#include "tilelite_config.h"

struct image_db;
struct tile_renderer;

struct tile_request {
  int sock_fd;
  tile req_tile;
};

struct image_write_task {
  image img;
  uint64_t position_hash;
  uint64_t image_hash;
};

struct tilelite {
  tilelite(const tilelite_config* conf);
  ~tilelite();
  void add_tile_request(tile_request req);
  void thread_job(image_db* db, const tilelite_config* conf);
  void image_write_job(image_db* db, const tilelite_config* conf);
  void queue_image_write(image_write_task task);
  void stop() { running = false; }
  std::vector<image_db*> databases;
  std::mutex queue_lock;
  std::queue<tile_request> pending_requests;
  std::atomic_bool running;
  LightweightSemaphore sema;
  std::vector<std::thread> threads;

  std::mutex img_queue_lock;
  std::queue<image_write_task> pending_img_writes;
  LightweightSemaphore img_write_sema;
  std::thread image_write_thread;
};

