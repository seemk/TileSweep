#include <fcntl.h>
#include <h2o.h>
#include <signal.h>
#include "hash/xxhash.h"
#include "image.h"
#include "image_db.h"
#include "tcp.h"
#include "tile_renderer.h"
#include "tl_log.h"
#include "tl_options.h"
#include "tl_tile.h"
#include "tl_time.h"
#include "tl_write_queue.h"
#include "cpu.h"

typedef enum { res_ok, res_failure } tilelite_result;

typedef struct {
  h2o_context_t super;
  void* user;
} tl_h2o_ctx;

typedef struct {
  h2o_accept_ctx_t accept_ctx;
  h2o_socket_t* socket;
  tl_h2o_ctx ctx;
  image_db* db;
  tile_renderer* renderer;
  int fd;
  size_t index;
  tl_write_queue* write_queue;
  int render_enabled;
} tilelite;

static struct {
  h2o_globalconf_t globalconf;
  tl_options* opt;
  size_t num_threads;
  tilelite* threads;
  int rendering;
} conf;

static void on_accept(h2o_socket_t* listener, const char* err) {
  if (err != NULL) {
    tl_log("accept error: %s", err);
    return;
  }

  h2o_socket_t* socket = h2o_evloop_socket_accept(listener);
  if (socket == NULL) {
    return;
  }

  tilelite* tl = (tilelite*)listener->data;
  h2o_accept(&tl->accept_ctx, socket);
}

int tilelite_init(tilelite* tl, const tl_options* opt) {
  if (strcmp(opt->rendering, "1") == 0) {
    tl->render_enabled = 1;
    tl->renderer =
        tile_renderer_create(opt->mapnik_xml, opt->plugins, opt->fonts);
    if (!tl->renderer) {
      return 1;
    }
  } else {
    tl->render_enabled = 0;
  }

  h2o_context_init(&tl->ctx.super, h2o_evloop_create(), &conf.globalconf);
  tl->ctx.user = tl;
  tl->accept_ctx.ctx = &tl->ctx.super;
  tl->accept_ctx.hosts = conf.globalconf.hosts;

  tl->socket = h2o_evloop_socket_create(tl->ctx.super.loop, tl->fd,
                                        H2O_SOCKET_FLAG_DONT_READ);
  tl->socket->data = tl;
  h2o_socket_read_start(tl->socket, on_accept);

  return 0;
}

static h2o_pathconf_t* register_handler(h2o_hostconf_t* conf, const char* path,
                                        int (*on_req)(h2o_handler_t*,
                                                      h2o_req_t*)) {
  h2o_pathconf_t* path_conf = h2o_config_register_path(conf, path, 0);
  h2o_handler_t* handler = h2o_create_handler(path_conf, sizeof(*handler));
  handler->on_req = on_req;

  return path_conf;
}

static int serve_job_get(h2o_handler_t* h, h2o_req_t* req) {
  printf("jobs get\n");
  tl_h2o_ctx* h2o = (tl_h2o_ctx*)req->conn->ctx;
  tilelite* tl = (tilelite*)h2o->user;
  h2o_generator_t gen = {NULL, NULL};
  return 0;
}

static int serve_job_post(h2o_handler_t* h, h2o_req_t* req) { return -1; }

static int serve_job_request(h2o_handler_t* handler, h2o_req_t* req) {
  if (h2o_memis(req->method.base, req->method.len, H2O_STRLIT("GET"))) {
    return serve_job_get(handler, req);
  } else if (h2o_memis(req->method.base, req->method.len, H2O_STRLIT("POST"))) {
    return serve_job_post(handler, req);
  }

  return -1;
}

static int serve_tile(h2o_handler_t* h, h2o_req_t* req) {
  int64_t req_start = tl_usec_now();

  tl_h2o_ctx* h2o = (tl_h2o_ctx*)req->conn->ctx;
  tilelite* tl = (tilelite*)h2o->user;
  h2o_generator_t gen = {NULL, NULL};
  tl_tile t = parse_tile(req->path.base, req->path.len);

  h2o_start_response(req, &gen);

  if (!tile_valid(&t) || (t.w != 256 && t.w != 512) ||
      (t.h != 256 && t.h != 512)) {
    req->res.status = 400;
    req->res.reason = "Bad Request";
    h2o_send(req, &req->entity, 1, 1);
    return 0;
  }

  uint64_t pos_hash = tile_hash(&t);
  image img = {.width = 0, .height = 0, .len = 0, .data = NULL};

  tilelite_result result = res_ok;
  int32_t existing = image_db_fetch(tl->db, pos_hash, t.w, t.h, &img);
  if (tl->render_enabled && !existing) {
    if (render_tile(tl->renderer, &t, &img)) {
      uint64_t image_hash = XXH64(img.data, img.len, 0);

      image write_img;
      write_img.width = img.width;
      write_img.height = img.height;
      write_img.len = img.len;
      write_img.data = (uint8_t*)calloc(write_img.len, 1);
      memcpy(write_img.data, img.data, img.len);
      tl_write_queue_push(tl->write_queue, t, write_img, image_hash);
    } else {
      result = res_failure;
    }
  }

  if (img.len > 0) {
    req->res.status = 200;
    req->res.reason = "OK";

    h2o_add_header(&req->pool, &req->res.headers, H2O_TOKEN_CONTENT_TYPE,
                   H2O_STRLIT("image/png"));

    char content_length[8];
    snprintf(content_length, 8, "%d", img.len);
    size_t header_length = strlen(content_length);
    h2o_add_header(&req->pool, &req->res.headers, H2O_TOKEN_CONTENT_LENGTH,
                   content_length, header_length);

    h2o_iovec_t body;
    body.base = (char*)img.data;
    body.len = img.len;
    h2o_send(req, &body, 1, 1);
    free(img.data);
  } else {
    if (result == res_failure) {
    } else {
      req->res.status = 204;
      req->res.reason = "No Content";
    }
    h2o_send(req, &req->entity, 1, 1);
  }

  int64_t req_time = tl_usec_now() - req_start;
  if (req_time > 1000) {
    tl_log("[%lu] [%d, %d, %d, %d, %d] (%d bytes) | %.2f ms [e: %d]", tl->index,
           t.w, t.h, t.z, t.x, t.y, img.len, req_time / 1000.0, existing);
  } else {
    tl_log("[%lu] [%d, %d, %d, %d, %d] (%d bytes) | %ld us [e: %d]", tl->index,
           t.w, t.h, t.z, t.x, t.y, img.len, req_time, existing);
  }

  return 0;
}

void* run_loop(void* arg) {
  long idx = (long)arg;

  tilelite* tl = &conf.threads[idx];
  tl->index = idx;

  if (tilelite_init(tl, conf.opt)) {
    tl_log("[%ld] failed to initialize", idx);
    return NULL;
  }

  tl_log("[%ld] ready", idx);
  while (h2o_evloop_run(tl->ctx.super.loop, INT32_MAX) == 0) {
  }

  return NULL;
}

void set_signal_handler(int sig_num, void (*handler)(int sig_num)) {
  struct sigaction action;

  memset(&action, 0, sizeof(action));

  sigemptyset(&action.sa_mask);
  action.sa_handler = handler;
  sigaction(sig_num, &action, NULL);
}

int main(int argc, char** argv) {
  set_signal_handler(SIGPIPE, SIG_IGN);

  tl_options opt = tl_options_parse(argc, argv);
  conf.opt = &opt;
  conf.num_threads = cpu_core_count();
  conf.threads = (tilelite*)calloc(conf.num_threads, sizeof(tilelite));
  conf.rendering = strcmp(opt.rendering, "1") == 0;

  h2o_config_init(&conf.globalconf);

  h2o_hostconf_t* hostconf = h2o_config_register_host(
      &conf.globalconf, h2o_iovec_init(H2O_STRLIT("default")), 65535);
  register_handler(hostconf, "/tile", serve_tile);
  register_handler(hostconf, "/jobs", serve_job_request);
  h2o_file_register(h2o_config_register_path(hostconf, "/", 0), "ui", NULL,
                    NULL, 0);

  tl_write_queue* write_queue = NULL;
  if (conf.rendering) {
    write_queue = tl_write_queue_create(image_db_open(opt.database));
  }

  int sock_fd = bind_tcp(opt.host, opt.port);
  if (sock_fd == -1) {
    tl_log("failed to bind socket");
    return 1;
  }

  for (size_t i = 0; i < conf.num_threads; i++) {
    tilelite* tl = &conf.threads[i];
    tl->db = image_db_open(opt.database);
    tl->write_queue = write_queue;
    tl->fd = sock_fd;
  }

  for (long i = 1; i < conf.num_threads; i++) {
    pthread_t tid;
    h2o_multithread_create_thread(&tid, NULL, &run_loop, (void*)i);
  }

  run_loop(NULL);

  return 0;
}
