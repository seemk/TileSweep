#pragma once

#include <thread>
#include <vector>
#include "thread/sema.h"
#include "thread/concurrentqueue.h"
#include "image.h"
#include "tile.h"
#include "tilelite_config.h"

template <typename T>
using tl_queue = moodycamel::ConcurrentQueue<T>;

struct image_db;

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
  void queue_tile_request(tile_request req);
  void thread_job(image_db* db, const tilelite_config* conf);
  void image_write_job(image_db* db, const tilelite_config* conf);
  void queue_image_write(image_write_task task);
  std::vector<image_db*> databases;
  LightweightSemaphore pending_requests_sema;
  tl_queue<tile_request> pending_requests;
  std::atomic_bool running;
  std::vector<std::thread> threads;

  LightweightSemaphore pending_img_writes_sema;
  tl_queue<image_write_task> pending_img_writes;
  std::thread image_write_thread;
};
