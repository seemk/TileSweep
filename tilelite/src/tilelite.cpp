#include "tilelite.h"
#include "image_db.h"
#include "tile_renderer.h"
#include <stdio.h>
#include <sys/socket.h>
#include "hash/MurmurHash2.h"

const uint64_t HASH_SEED = 0x1F0D3804;

void send_tile(int fd, const image* img) {
  int header_bytes_sent = send(fd, &img->len, sizeof(int), 0);
  if (header_bytes_sent == -1) {
    perror("failure sending image size: ");
    return;
  }

  const int bytes_to_send = img->len;

  int bytes_left = bytes_to_send;

  while (bytes_left > 0) {
    int bytes_sent = send(fd, img->data, bytes_left, 0);

    if (bytes_sent == -1) {
      perror("send to client fail: ");
      break;
    }

    bytes_left -= bytes_sent;
  }
}

tilelite::tilelite(const tilelite_config* conf)
  : running(true) {

  int num_threads = std::stoi(conf->at("threads"));
  if (num_threads < 1) num_threads = 1;

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
  image_write_thread = std::thread([=] { this->image_write_job(write_db, conf); });
}

void tilelite::add_tile_request(tile_request req) {
  std::lock_guard<std::mutex> lock(queue_lock);
  pending_requests.push(req);
  sema.signal(1);
}

void tilelite::queue_image_write(image_write_task task) {
  std::lock_guard<std::mutex> lock(img_queue_lock);
  pending_img_writes.push(task);
  img_write_sema.signal(1);
}

tilelite::~tilelite() {
  running = false;

  sema.signal(threads.size());
  img_write_sema.signal(1);
  for (std::thread& t : threads) {
    t.join();
  }

  image_write_thread.join();

  for (image_db* db : databases) {
    image_db_close(db);
  }
}

void tilelite::thread_job(image_db* db, const tilelite_config* conf) {
  std::string mapnik_xml_path = conf->at("mapnik_xml");

  tile_renderer renderer;
  tile_renderer_init(&renderer, mapnik_xml_path.c_str());

  auto id = std::this_thread::get_id();
  while (running) {
    sema.wait();

    std::unique_lock<std::mutex> lock(queue_lock);

    if (pending_requests.empty()) continue;

    tile_request req = pending_requests.front();
    pending_requests.pop();
    lock.unlock();

    uint64_t pos_hash = req.req_tile.hash();
    image img;

    bool existing = image_db_fetch(db, pos_hash, &img);
    if (existing) {
      send_tile(req.sock_fd, &img);
    } else {
      if (render_tile(&renderer, &req.req_tile, &img)) {
        send_tile(req.sock_fd, &img);

        uint64_t image_hash = MurmurHash2(img.data, img.len, HASH_SEED);

        queue_image_write({img, pos_hash, image_hash});
      }
    }
  }

  tile_renderer_destroy(&renderer);
}

void tilelite::image_write_job(image_db* db, const tilelite_config* conf) {
  while (running) {
    img_write_sema.wait();

    std::lock_guard<std::mutex> lock(img_queue_lock);
    
    while (!pending_img_writes.empty()) {
      image_write_task task = pending_img_writes.front();
      pending_img_writes.pop();

      image_db_add_image(db, &task.img, task.image_hash);
      image_db_add_position(db, task.position_hash, task.image_hash);

      free(task.img.data);
    }
  }
}
