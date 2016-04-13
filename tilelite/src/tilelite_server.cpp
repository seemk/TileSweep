#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <signal.h>
#include "ini/ini.h"
#include "tilelite.h"
#include "tile_renderer.h"
#include "tcp.h"

#ifdef TILELITE_EPOLL
#include "ev_loop_epoll.h"
#elif TILELITE_KQUEUE
#include "ev_loop_kqueue.h"
#endif

void set_signal_handler(int sig_num, void (*handler)(int sig_num)) {
  struct sigaction action;

  memset(&action, 0, sizeof(action));

  sigemptyset(&action.sa_mask);
  action.sa_handler = handler;
  sigaction(sig_num, &action, NULL);
}

int ini_parse_callback(void* user, const char* section, const char* name,
                       const char* value) {
  tilelite_config* conf = (tilelite_config*)user;

  if (strcmp(name, "plugins") == 0) {
    register_plugins(value);
  } else if (strcmp(name, "fonts") == 0) {
    register_fonts(value);
  } else { 
    (*conf)[name] = value;
  }

  return 1;
}

void set_defaults(tilelite_config* conf) {
  auto set_key = [conf](const char* key, const char* value) {
    if (conf->count(key) == 0) (*conf)[key] = value;
  };

  set_key("threads", "1");
  set_key("tile_db", "tiles.db");
  set_key("port", "9567");
}


int main(int argc, char** argv) {
  tilelite_config conf;

  if (ini_parse("conf.ini", ini_parse_callback, &conf) < 0) {
    fprintf(stderr, "failed load configuration file\n");
    return 1;
  }

  set_defaults(&conf);
  set_signal_handler(SIGPIPE, SIG_IGN);

  int sfd = bind_tcp(conf["port"].c_str());
  if (sfd == -1) {
    return 1;
  }

  if (listen(sfd, SOMAXCONN) == -1) {
    perror("listen");
    return 1;
  }

  auto context = std::unique_ptr<tilelite>(new tilelite(&conf));

#ifdef TILELITE_EPOLL
  ev_loop_epoll loop;
  ev_loop_epoll_init(&loop, sfd);
#elif TILELITE_KQUEUE
  ev_loop_kqueue loop;
  ev_loop_kqueue_init(&loop, sfd);
#endif

  loop.user = context.get();

  auto read_callback = [](int fd, const char* data, int len, void* user) {
    tilelite* ctx = (tilelite*)user;
    tile coord = parse_tile(data, len);
    ctx->queue_tile_request({fd, coord});
  };

#ifdef TILELITE_EPOLL
  ev_loop_epoll_run(&loop, read_callback);
#elif TILELITE_KQUEUE
  ev_loop_kqueue_run(&loop, read_callback);
#endif

  return 0;
}
