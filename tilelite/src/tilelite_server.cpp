#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <signal.h>
#include "ini/ini.h"
#include "tilelite.h"
#include "tile_renderer.h"
#include "tcp.h"
#include "tl_time.h"
#include "tl_math.h"
#include <rapidjson/document.h>
#include <mapnik/debug.hpp>

#ifdef TILELITE_EPOLL
#include "ev_loop_epoll.h"
#elif TILELITE_KQUEUE
#include "ev_loop_kqueue.h"
#endif

namespace rj = rapidjson;

void read_tile_req(rj::Value& doc, tl_request* req) {
  tl_tile* tile = &req->as.tile;
  tile->x = doc["x"].GetInt();
  tile->y = doc["y"].GetInt();
  tile->z = doc["z"].GetInt();
  tile->w = doc["w"].GetInt();
  tile->h = doc["h"].GetInt();
}

void read_prerender_req(rj::Value& doc, tl_request* req) {
  tl_prerender* prerender = &req->as.prerender;

  // TODO: Schema validation
  const rj::Value& zoom_levels = doc["zoom"];

  if (zoom_levels.IsArray()) {
    prerender->num_zoom_levels = zoom_levels.Size() > 19 ? 19 : zoom_levels.Size();
    for (int i = 0; i < prerender->num_zoom_levels; i++) {
      prerender->zoom[i] = tl_clamp(zoom_levels[i].GetInt(), 0, 18);
    }
  }

  const rj::Value& bounds = doc["bounds"];

  if (bounds.IsArray()) {
    prerender->num_points = bounds.Size();
    prerender->points = (vec2d*)calloc(prerender->num_points, sizeof(vec2d));
    for (int i = 0; i < prerender->num_points; i++) {
      const rj::Value& coord = bounds[i];
      
      if (coord.IsArray()) {
        prerender->points[i].x = coord[0].GetDouble();
        prerender->points[i].y = coord[1].GetDouble();
      }
    }
  }

  prerender->width = doc["width"].GetInt();
  prerender->height = doc["height"].GetInt();
}

void set_signal_handler(int sig_num, void (*handler)(int sig_num)) {
  struct sigaction action;

  memset(&action, 0, sizeof(action));

  sigemptyset(&action.sa_mask);
  action.sa_handler = handler;
  sigaction(sig_num, &action, nullptr);
}

int ini_parse_callback(void* user, const char* section, const char* name,
                       const char* value) {
  tilelite_config* conf = (tilelite_config*)user;
  (*conf)[name] = value;

  return 1;
}

void set_defaults(tilelite_config* conf) {
  auto set_key = [conf](const char* key, const char* value) {
    if (conf->count(key) == 0) (*conf)[key] = value;
  };

  set_key("plugins", "");
  set_key("fonts", "");
  set_key("threads", "1");
  set_key("tile_db", "tiles.db");
  set_key("port", "9567");
  set_key("rendering", "1");
}

tl_request read_request(const char* data, int len) {
  tl_request req = {0};
  rj::Document doc;
  doc.Parse(data, len);

  printf("%.*s\n", len, data);

  if (doc.HasParseError() || !doc.IsObject()) {
    printf("invalid JSON\n");
    return req;
  }

  if (!(doc.HasMember("type") && doc.HasMember("content") &&
        doc["type"].IsNumber() && doc["content"].IsObject())) {
    printf("invalid request\n");
    return req;
  }

  req.type = tl_request_type(doc["type"].GetUint64());

  if (req.type == rq_invalid || req.type >= RQ_COUNT) {
    printf("invalid request type\n");
    req.type = rq_invalid;
    return req;
  }

  rj::Value& content = doc["content"];

  switch (req.type) {
    case rq_tile:
      read_tile_req(content, &req);
      break;
    case rq_prerender:
      read_prerender_req(content, &req);
      break;
    default:
      break;
  }

  return req;
}

int main(int argc, char** argv) {
  tilelite_config conf;

  if (ini_parse("conf.ini", ini_parse_callback, &conf) < 0) {
    fprintf(stderr, "failed load configuration file\n");
    return 1;
  }

  if (conf["rendering"] == "1") {
    mapnik::logger::instance().set_severity(mapnik::logger::none);
    register_plugins(conf["plugins"].c_str());
    register_fonts(conf["fonts"].c_str());
  }

  set_defaults(&conf);
  set_signal_handler(SIGPIPE, SIG_IGN);
  set_signal_handler(SIGINT, SIG_IGN);

  int sfd = bind_tcp(conf["port"].c_str());
  if (sfd == -1) {
    return 1;
  }

  if (listen(sfd, SOMAXCONN) == -1) {
    perror("listen");
    return 1;
  }

#ifdef TILELITE_EPOLL
  ev_loop_epoll loop;
  ev_loop_epoll_init(&loop, sfd);
#elif TILELITE_KQUEUE
  ev_loop_kqueue loop;
  ev_loop_kqueue_init(&loop, sfd);
#endif

  auto context = std::unique_ptr<tilelite>(new tilelite(&conf));
  loop.user = context.get();

  auto read_callback = [](int fd, const char* data, int len, void* user) {
    tilelite* ctx = (tilelite*)user;

    tl_request req = read_request(data, len);
    if (req.type != rq_invalid) {
      req.client_fd = fd;
      req.request_time = tl_usec_now();
      ctx->queue_tile_request(req);
    }
  };

#ifdef TILELITE_EPOLL
  ev_loop_epoll_run(&loop, read_callback);
#elif TILELITE_KQUEUE
  ev_loop_kqueue_run(&loop, read_callback);
#endif

  return 0;
}
