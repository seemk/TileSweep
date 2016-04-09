#include <stdlib.h>
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
#include "image_db.h"
#include <string>
#include "tile_renderer.h"
#include "ini/ini.h"
#include "tile.h"
#include "image.h"
#include "hash/MurmurHash2.h"

const uint64_t HASH_SEED = 0x1F0D3804;

void set_signal_handler(int sig_num, void (*handler)(int sig_num)) {
  struct sigaction action;

  memset(&action, 0, sizeof(action));

  sigemptyset(&action.sa_mask);
  action.sa_handler = handler;
  sigaction(sig_num, &action, NULL);
}

uint64_t position_hash(uint64_t z, uint64_t x, uint64_t y) {
  return (z << 40) | (x << 20) | y;
}

int atoi_len(const char* s, int len) {
  int n = 0;

  for (int i = len; i > 0; i--) {
    n = n * 10 + (*s++ - '0');
  }

  return n;
}

struct tile_request {
  struct sockaddr_storage remote_addr;
  socklen_t remote_addr_len;
  tile tile_dim;
};

struct tilelite {
  tile_renderer renderer;
};

int ini_parse_callback(void* user, const char* section, const char* name,
                       const char* value) {
  tilelite* context = (tilelite*)user;

  if (strcmp(name, "plugins") == 0) {
    register_plugins(value);
  } else if (strcmp(name, "fonts") == 0) {
    register_fonts(value);
  } else if (strcmp(name, "mapnik_xml") == 0) {
    tile_renderer_init(&context->renderer, value);
  }

  return 1;
}

int bind_tcp(const char* port) {
  struct addrinfo hints;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  struct addrinfo* results;
  int rv = getaddrinfo(NULL, port, &hints, &results);

  if (rv != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return -1;
  }

  int fd = -1;

  struct addrinfo* result;
  for (result = results; result != NULL; result = result->ai_next) {
    fd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (fd == -1) {
      continue;
    }

    int enable = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
      fprintf(stderr, "setsockopt(SO_REUSEADDR) failed");
    }

    if (bind(fd, result->ai_addr, result->ai_addrlen) == 0) {
      break;
    }

    close(fd);
  }

  if (result == NULL) {
    fprintf(stderr, "failed to bind socket\n");
    return -1;
  }

  freeaddrinfo(result);

  return fd;
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

void send_tile(int fd, const image* img) {
  send(fd, &img->len, sizeof(int), 0);

  const int bytes_to_send = img->len;
  printf("Bytes to send: %d\n", bytes_to_send);

  int bytes_left = bytes_to_send;

  while (bytes_left > 0) {
    printf("Sending bytes %d\n", bytes_left);
    int bytes_sent = send(fd, img->data, bytes_left, 0);
    printf("Bytes written: %d\n", bytes_sent);

    if (bytes_sent == -1) {
      perror("send to client fail");
      break;
    }

    bytes_left -= bytes_sent;
  }
}

int main(int argc, char** argv) {
  tilelite context;

  if (ini_parse("conf.ini", ini_parse_callback, &context) < 0 ||
      context.renderer.map == NULL) {
    fprintf(stderr, "failed load configuration file\n");
    return 1;
  }

  image_db* db = image_db_open("image.db");

  if (!db) {
    fprintf(stderr, "failed to open DB\n");
    return 1;
  }

  set_signal_handler(SIGPIPE, SIG_IGN);

  int sfd = bind_tcp("9567");
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

  int efd = epoll_create1(0);

  if (efd == -1) {
    perror("epoll create");
    return 1;
  }

  struct epoll_event event;
  event.data.fd = sfd;
  event.events = EPOLLIN | EPOLLET;

  res = epoll_ctl(efd, EPOLL_CTL_ADD, sfd, &event);

  if (res == -1) {
    perror("epoll ctl");
    return 1;
  }

  const int max_events = 128;
  struct epoll_event* events =
      (struct epoll_event*)calloc(max_events, sizeof(event));

  bool running = true;
  while (running) {
    int n = epoll_wait(efd, events, max_events, -1);

    for (int i = 0; i < n; i++) {
      struct epoll_event* ev = &events[i];
      uint32_t ev_flags = events[i].events;
      if (ev_flags & EPOLLERR || ev_flags & EPOLLHUP || !(ev_flags & EPOLLIN)) {
        fprintf(stderr, "epoll error\n");
        close(events[i].data.fd);
        continue;
      } else if (sfd == ev->data.fd) {
        while (1) {
          struct sockaddr in_addr;
          socklen_t in_len;
          int client_fd = accept(sfd, &in_addr, &in_len);

          if (client_fd == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
              break;
            } else {
              perror("accept");
              break;
            }
          }

          char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
          res = getnameinfo(&in_addr, in_len, hbuf, sizeof(hbuf), sbuf,
                            sizeof(sbuf), NI_NUMERICHOST | NI_NUMERICSERV);

          if (res == 0) {
            printf("Connection from %s:%s\n", hbuf, sbuf);
          }

          res = set_nonblocking(client_fd);

          if (res == -1) {
            fprintf(stderr, "failed to set client nonblocking\n");
            break;
          }

          event.data.fd = client_fd;
          event.events = EPOLLIN | EPOLLET;

          res = epoll_ctl(efd, EPOLL_CTL_ADD, client_fd, &event);

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

          printf("Request: %s\n", buf);

          tile coord = parse_tile(buf, num_bytes);

          uint64_t pos_hash = position_hash(coord.z, coord.x, coord.y);
          image img;
          img.data = NULL;
          bool existing = image_db_fetch(db, pos_hash, &img);
          if (existing) {
            printf("Retrieved image, len: %d\n", img.len);
          } else {
            render_tile(&context.renderer, &coord, &img);
            uint64_t image_hash = MurmurHash2(img.data, img.len, HASH_SEED);
            bool add_img = image_db_add_image(db, &img, image_hash);
            bool add_pos = image_db_add_position(db, pos_hash, image_hash);
            printf("Rendered image, len: %d\n", img.len);
          }

          send_tile(ev->data.fd, &img);

          if (img.data) {
            free(img.data);
          }
        }
      }
    }
  }

  printf("Quitting\n");
  close(sfd);

  image_db_close(db);
  free(events);

  return 0;
}
