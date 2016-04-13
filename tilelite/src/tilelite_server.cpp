#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/signalfd.h>
#include "ini/ini.h"
#include "tilelite.h"
#include "tile_renderer.h"
#include "tcp.h"

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

int set_nonblocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);

  if (flags == -1) {
    perror("get fcntl");
    return -1;
  }

  flags |= O_NONBLOCK;

  int res = fcntl(fd, F_SETFL, flags);

  if (res == -1) {
    perror("set fcntl");
    return -1;
  }

  return 0;
}

void set_defaults(tilelite_config* conf) {
  auto set_key = [conf](const char* key, const char* value) {
    if (conf->count(key) == 0) (*conf)[key] = value;
  };

  set_key("threads", "1");
  set_key("tile_db", "tiles.db");
  set_key("port", "9567");
}

const int MAX_EVENTS = 128;

struct ev_loop_epoll {
  int socket;
  int efd;
  void* user = NULL;
  struct epoll_event event;
  struct epoll_event events[MAX_EVENTS];
  int sigfd;
};

bool ev_loop_epoll_init(ev_loop_epoll* loop, int socket) {
  loop->socket = socket;
  loop->efd = epoll_create1(0);

  if (loop->efd == -1) {
    perror("epoll create");
    return false;
  }

  loop->event.data.fd = socket;
  loop->event.events = EPOLLIN | EPOLLET;

  int res = epoll_ctl(loop->efd, EPOLL_CTL_ADD, socket, &loop->event);

  if (res == -1) {
    perror("epoll ctl");
    return false;
  }

  sigset_t signals;
  sigemptyset(&signals);
  sigaddset(&signals, SIGINT);
  sigprocmask(SIG_BLOCK, &signals, NULL);

  int sig_fd = signalfd(-1, &signals, SFD_NONBLOCK);

  if (sig_fd == -1) {
    perror("signalfd creation: ");
    return false;
  }

  struct epoll_event sig_event;
  sig_event.data.fd = sig_fd;
  sig_event.events = EPOLLIN;
  if (epoll_ctl(loop->efd, EPOLL_CTL_ADD, sig_fd, &sig_event) == -1) {
    perror("signalfd epoll failure: ");
    return false;
  }

  loop->sigfd = sig_fd;

  return true;
};

void ev_loop_epoll_run(ev_loop_epoll* loop, void (*cb)(int, const char*, int, void*)) {
  int efd = loop->efd;

  for(;;) {
    int n = epoll_wait(loop->efd, loop->events, MAX_EVENTS, -1);

    for (int i = 0; i < n; i++) {
      struct epoll_event* ev = &loop->events[i];
      uint32_t ev_flags = loop->events[i].events;

      if (ev->data.fd == loop->sigfd) {
        struct signalfd_siginfo info;
        int signal_bytes = read(loop->sigfd, &info, sizeof(info));
        assert(signal_bytes == sizeof(info));

        if (info.ssi_signo == SIGINT) {
          close(loop->socket);
          close(loop->efd);
          return;
        }

        continue;
      }

      if (ev_flags & EPOLLERR || ev_flags & EPOLLHUP || !(ev_flags & EPOLLIN)) {
        close(loop->events[i].data.fd);
        continue;
      } else if (loop->socket == ev->data.fd) {
        for(;;) {
          struct sockaddr in_addr;
          socklen_t in_len = sizeof(in_addr);
          int client_fd = accept(loop->socket, &in_addr, &in_len);

          if (client_fd == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
              break;
            } else {
              perror("error accepting connection: ");
              break;
            }
          }

          char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
          int res = getnameinfo(&in_addr, in_len, hbuf, sizeof(hbuf), sbuf,
                            sizeof(sbuf), NI_NUMERICHOST | NI_NUMERICSERV);

          if (res == 0) {
            printf("connection from %s:%s\n", hbuf, sbuf);
          }

          res = set_nonblocking(client_fd);

          if (res == -1) {
            fprintf(stderr, "failed to set client nonblocking\n");
            break;
          }

          struct epoll_event event;
          event.data.fd = client_fd;
          event.events = EPOLLIN | EPOLLET;

          res = epoll_ctl(loop->efd, EPOLL_CTL_ADD, client_fd, &event);

          if (res == -1) {
            perror("epoll client add");
            break;
          }
        }
      } else {
        const int read_buf_len = 512;
        char buf[read_buf_len + 1];
        for (;;) {
          ssize_t num_bytes = read(ev->data.fd, buf, read_buf_len);
          if (num_bytes == -1) {
            if (errno != EAGAIN) {
              perror("fd read");
              close(ev->data.fd);
            }
            break;
          } else if (num_bytes == 0) {
            printf("Client on FD %d closed\n", ev->data.fd);
            close(ev->data.fd); 
            break;
          }

          buf[num_bytes] = '\0';

          cb(ev->data.fd, buf, num_bytes, loop->user);
        }
      }
    }
  }
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

  int res = set_nonblocking(sfd);
  if (res == -1) {
    return 1;
  }

  res = listen(sfd, SOMAXCONN);
  if (res == -1) {
    perror("listen");
    return 1;
  }

  ev_loop_epoll loop;
  ev_loop_epoll_init(&loop, sfd);

  auto context = std::unique_ptr<tilelite>(new tilelite(&conf));

  loop.user = context.get();

  ev_loop_epoll_run(&loop, [](int fd, const char* data, int len, void* user) {
    tilelite* ctx = (tilelite*)user;
    tile coord = parse_tile(data, len);
    ctx->queue_tile_request({fd, coord});
  });

  return 0;
}
