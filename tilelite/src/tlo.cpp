#include <ctype.h>
#include <h2o.h>
#include "hash/xxhash.h"
#include "image.h"
#include "image_db.h"
#include "tcp.h"
#include "tile_renderer.h"
#include "tilelite_config.h"
#include "tl_request.h"
#include "tl_time.h"
#include "tl_log.h"

struct tilelite {
  h2o_accept_ctx_t accept_ctx;
  image_db* db;
  tile_renderer renderer;
};

struct tilelite_handler {
  h2o_handler_t super;
  void* user;
};

void tilelite_init(tilelite* tl, const tilelite_config* conf) {
  tl->db = image_db_open(conf->at("tile_db").c_str());

  if (conf->at("rendering") == "1") {
    tile_renderer_init(&tl->renderer, conf->at("mapnik_xml").c_str(),
                       conf->at("plugins").c_str(), conf->at("fonts").c_str());
  }
}

static int strntoi(const char* s, size_t len) {
  int n = 0;

  for (size_t i = 0; i < len; i++) {
    n = n * 10 + s[i] - '0';
  }

  return n;
}

static tl_tile parse_tile(const char* s, size_t len) {
  size_t begin = 0;
  size_t end = 0;
  int part = 0;
  int parsing_num = 0;
  int params[5] = {-1};

  for (size_t i = 0; i < len; i++) {
    if (isdigit(s[i])) {
      if (!parsing_num) {
        begin = i;
        parsing_num = 1;
      }
      end = i;
    } else {
      if (parsing_num) {
        params[part++] = int(strntoi(&s[begin], end - begin + 1));
        parsing_num = 0;
      }
    }

    if (i == len - 1 && parsing_num) {
      params[part] = int(strntoi(&s[begin], end - begin + 1));
    }
  }

  tl_tile t;
  t.w = params[0];
  t.h = params[1];
  t.x = params[2];
  t.y = params[3];
  t.z = params[4];

  return t;
}

static h2o_pathconf_t* RegisterHandler(h2o_hostconf_t* conf, const char* path,
                                       int (*onReq)(h2o_handler_t*, h2o_req_t*),
                                       void* user) {
  h2o_pathconf_t* pathConf = h2o_config_register_path(conf, path, 0);
  tilelite_handler* handler =
      (tilelite_handler*)h2o_create_handler(pathConf, sizeof(*handler));
  handler->super.on_req = onReq;
  handler->user = user;

  return pathConf;
}

static int ServeTile(h2o_handler_t* self, h2o_req_t* req) {
  int64_t req_start = tl_usec_now();
  tilelite_handler* handler = (tilelite_handler*)self;
  h2o_generator_t gen = {NULL, NULL};
  req->res.status = 200;
  req->res.reason = "OK";
  tl_tile t = parse_tile(req->path.base, req->path.len);

  uint64_t pos_hash = t.hash();
  image img;

  tilelite* tl = (tilelite*)handler->user;
  bool existing = image_db_fetch(tl->db, pos_hash, t.w, t.h, &img);
  if (!existing) {
    if (render_tile(&tl->renderer, &t, &img)) {
      uint64_t image_hash = XXH64(img.data, img.len, 0);
      image_db_add_image(tl->db, &img, image_hash);
      image_db_add_position(tl->db, pos_hash, image_hash);
    }
  }

  h2o_start_response(req, &gen);
  if (img.len > 0) {
    h2o_add_header(&req->pool, &req->res.headers, H2O_TOKEN_CONTENT_TYPE,
                   H2O_STRLIT("image/png"));
    h2o_iovec_t body;
    body.base = (char*)img.data;
    body.len = img.len;
    h2o_send(req, &body, 1, 1);

    free(img.data);
  } else {
    h2o_send(req, &req->entity, 1, 1);
  }

  int64_t req_time = tl_usec_now() - req_start;
  if (req_time > 1000) {
    tl_log("[%d, %d, %d, %d, %d] (%d bytes) | %.2f ms", t.w, t.h, t.z, t.x, t.y,
           img.len, double(req_time) / 1000.0);
  } else {
    tl_log("[%d, %d, %d, %d, %d] (%d bytes) | %lld us", t.w, t.h, t.z, t.x, t.y,
           img.len, req_time);
  }

  return 0;
}

static void on_accept(h2o_socket_t* listener, const char* err) {
  if (err != NULL) {
    printf("Accept error: %s\n", err);
    return;
  }

  h2o_socket_t* socket = h2o_evloop_socket_accept(listener);
  if (socket == NULL) {
    return;
  }

  tilelite* tl = (tilelite*)listener->data;
  h2o_accept(&tl->accept_ctx, socket);
}

int main(int argc, char** argv) {
  tilelite_config tl_config = parse_args(argc, argv);

  h2o_globalconf_t config;
  h2o_config_init(&config);

  tilelite tl = {0};
  tilelite_init(&tl, &tl_config);

  h2o_hostconf_t* hostconf = h2o_config_register_host(
      &config, h2o_iovec_init(H2O_STRLIT("default")), 65535);
  RegisterHandler(hostconf, "/", ServeTile, &tl);

  h2o_context_t ctx;
  h2o_context_init(&ctx, h2o_evloop_create(), &config);

  tl.accept_ctx.ctx = &ctx;
  tl.accept_ctx.hosts = config.hosts;

  int sock_fd = bind_tcp("127.0.0.1", "9567");
  if (sock_fd == -1) {
    return 1;
  }

  h2o_socket_t* socket =
      h2o_evloop_socket_create(ctx.loop, sock_fd, H2O_SOCKET_FLAG_DONT_READ);
  socket->data = &tl;
  h2o_socket_read_start(socket, on_accept);

  while (h2o_evloop_run(ctx.loop) == 0) {
  }

  return 0;
}
