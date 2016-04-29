#include "ev_loop_epoll.h"
#include <sys/signalfd.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <assert.h>

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

bool ev_loop_epoll_init(ev_loop_epoll* loop, int socket) {
  int res = set_nonblocking(socket);

  if (res == -1) {
    return false;
  }

  loop->socket = socket;
  loop->efd = epoll_create1(0);

  if (loop->efd == -1) {
    perror("epoll create");
    return false;
  }

  struct epoll_event ev;
  ev.data.fd = socket;
  ev.events = EPOLLIN | EPOLLET;
  res = epoll_ctl(loop->efd, EPOLL_CTL_ADD, socket, &ev);

  if (res == -1) {
    perror("epoll ctl");
    return false;
  }

  sigset_t signals;
  sigemptyset(&signals);
  sigaddset(&signals, SIGINT);
  sigprocmask(SIG_BLOCK, &signals, nullptr);

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

void ev_loop_epoll_run(ev_loop_epoll* loop,
                       void (*cb)(int, const char*, int, void*)) {
  int efd = loop->efd;

  for (;;) {
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
        for (;;) {
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
        const int read_buf_len = 4096;
        int total_read = 0;
        int remain = read_buf_len;
        char buf[read_buf_len + 1];
        for (;;) {
          ssize_t num_bytes = read(ev->data.fd, buf + total_read, remain);
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
          } else {
            total_read += num_bytes;
            remain -= num_bytes;
          }
        }
      
        buf[total_read] = '\0';
        cb(ev->data.fd, buf, total_read, loop->user);
      }
    }
  }
}
