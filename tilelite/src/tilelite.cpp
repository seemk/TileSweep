#include "tilelite.h"
#include "image_db.h"
#include "tile_renderer.h"
#include "hash/MurmurHash2.h"
#include "tl_time.h"
#include "tl_math.h"
#include "prerender.h"
#include "send.h"

const uint64_t HASH_SEED = 0x1F0D3804;

void process_simple_render(tilelite* context, image_db* db, tile_renderer* renderer, tl_tile t) {
  uint64_t pos_hash = t.hash();

  // TODO: Check existence without reading and overwrite
  image img;
  bool existing = image_db_fetch(db, pos_hash, t.w, t.h, &img);

  if (existing) {
    free(img.data);
    return;
  }

  if (render_tile(renderer, &t, &img)) {
    uint64_t image_hash = MurmurHash2(img.data, img.len, HASH_SEED);
    context->queue_image_write({img, pos_hash, image_hash});
  }
}

void process_prerender(tilelite* context, tl_prerender prerender) {
  printf("prerender req, w: %d h: %d num_points: %d\n\tzoom: ", prerender.width, prerender.height, prerender.num_points);

  for (int i = 0; i < prerender.num_zoom_levels; i++) {
    printf("%d ", prerender.zoom[i]);
  }

  printf("\n\tcoordinates: ");

  for (int i = 0; i < prerender.num_points; i++) {
    printf("(%.4f, %.4f) ", prerender.points[i].x, prerender.points[i].y);
  }

  printf("\n");

  const int num_points = prerender.num_points;
  std::vector<vec2d> coordinates;
  coordinates.reserve(num_points);

  for (int i = 0; i < num_points; i++) {
    coordinates.push_back(prerender.points[i]);
  }

  uint64_t total_indices = 0;
  for (int i = 0; i < prerender.num_zoom_levels; i++) {
    std::vector<vec2i> xyz_coordinates(num_points);
    int z = prerender.zoom[i];
    latlon_to_xyz(coordinates.data(), num_points, z, xyz_coordinates.data());
    std::vector<vec2i> prerender_indices = make_prerender_indices(xyz_coordinates.data(), num_points);

    total_indices += prerender_indices.size();

    for (auto p : prerender_indices) {
      tl_request req;
      req.request_time = tl_usec_now();
      req.client_fd = -1;
      req.type = rq_prerender_img;

      tl_tile* tile = &req.as.tile;
      tile->x = p.x;
      tile->y = p.y;
      tile->z = z;
      tile->w = prerender.width;
      tile->h = prerender.height;
      context->queue_tile_request(req);
    }
  }

  printf("prerender queued; total indices: %llu\n", total_indices);

  free(prerender.points);
}

tilelite::tilelite(const tilelite_config* conf) : running(true) {
  int num_threads = std::stoi(conf->at("threads"));
  if (num_threads < 1) num_threads = 1;

  rendering_enabled = conf->at("rendering") == "1";

  std::string db_name = conf->at("tile_db");
  for (int i = 0; i < num_threads + 1; i++) {
    image_db* db = image_db_open(db_name.c_str());
    databases.push_back(db);
  }

  for (int i = 0; i < num_threads; i++) {
    image_db* db = databases[i];
    threads.emplace_back([=] { this->thread_job(db, conf); });
  }

  image_db* write_db = databases[num_threads];
  image_write_thread =
      std::thread([=] { this->image_write_job(write_db, conf); });
}

void tilelite::queue_tile_request(tl_request req) {
  pending_requests.enqueue(req);
  pending_requests_sema.signal(1);
}

void tilelite::queue_image_write(image_write_task task) {
  pending_img_writes.enqueue(task);
  pending_img_writes_sema.signal(1);
}

tilelite::~tilelite() {
  running = false;

  pending_requests_sema.signal(threads.size());
  for (std::thread& t : threads) {
    t.join();
  }

  pending_img_writes_sema.signal(1);
  image_write_thread.join();

  for (image_db* db : databases) {
    image_db_close(db);
  }
}

void tilelite::thread_job(image_db* db, const tilelite_config* conf) {
  std::string mapnik_xml_path = conf->at("mapnik_xml");

  tile_renderer renderer;
  if (conf->at("rendering") == "1") {
    if (!tile_renderer_init(&renderer, mapnik_xml_path.c_str())) {
      fprintf(stderr, "failed to initialize renderer\n");
      return;
    }
  }

  while (running) {
    pending_requests_sema.wait();

    tl_request req;
    while (pending_requests.try_dequeue(req)) {
      switch (req.type) {
        case rq_tile:
          {
            tl_tile t = req.as.tile;
            uint64_t pos_hash = t.hash();
            image img;
            bool existing = image_db_fetch(db, pos_hash, t.w, t.h, &img);
            if (existing) {
              send_image(req.client_fd, &img);
              int64_t processing_time_us = tl_usec_now() - req.request_time;
              if (processing_time_us > 1000) {
                printf("[%d, %d, %d, %d, %d] (%d bytes) | %.2f ms\n", t.w, t.h, t.z,
                       t.x, t.y, img.len, double(processing_time_us) / 1000.0);
              } else {
                printf("[%d, %d, %d, %d, %d] (%d bytes) | %lld us\n", t.w, t.h, t.z, t.x,
                       t.y, img.len, processing_time_us);
              }
              free(img.data);
            } else {
              if (rendering_enabled && render_tile(&renderer, &t, &img)) {
                send_image(req.client_fd, &img);
                uint64_t image_hash = MurmurHash2(img.data, img.len, HASH_SEED);
                queue_image_write({img, pos_hash, image_hash});
              } else {
                send_status(req.client_fd, tl_no_tile);
              }
            }
          }
          break;
        case rq_prerender:
          process_prerender(this, req.as.prerender); 
          send_status(req.client_fd, tl_ok);
          break;
        case rq_prerender_img:
          process_simple_render(this, db, &renderer, req.as.tile);
          break;
        default:
          break;
      }
    }
  }

  tile_renderer_destroy(&renderer);
}

void tilelite::image_write_job(image_db* db, const tilelite_config* conf) {
  while (running) {
    pending_img_writes_sema.wait();

    image_write_task task;
    while (pending_img_writes.try_dequeue(task)) {
      image_db_add_image(db, &task.img, task.image_hash);
      image_db_add_position(db, task.position_hash, task.image_hash);
      free(task.img.data);
    }
  }
}
