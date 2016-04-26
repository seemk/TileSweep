#include "ev_loop_kqueue.h"
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>

bool ev_loop_kqueue_init(ev_loop_kqueue* loop, int socket) {
  loop->socket = socket;
  loop->kq = kqueue();

  struct kevent ev_read;
  EV_SET(&ev_read, socket, EVFILT_READ, EV_ADD, 0, 0, nullptr);
  if (kevent(loop->kq, &ev_read, 1, nullptr, 0, nullptr) == -1) {
    return false;
  }

  struct kevent sig_ev;
  EV_SET(&sig_ev, SIGINT, EVFILT_SIGNAL, EV_ADD, 0, 0, nullptr);
  if (kevent(loop->kq, &sig_ev, 1, nullptr, 0, nullptr) == -1) {
    return false;
  }

  return true;
}

void ev_loop_kqueue_run(ev_loop_kqueue* loop, void (*cb)(int, const char*, int, void*)) {
  int kq = loop->kq;
  for (;;) {
    int n = kevent(kq, nullptr, 0, loop->ev_list, 128, nullptr);
    if (n < 1) {
      printf("kevent error\n");
    }

    for (int i = 0; i < n; i++) {
      struct kevent* ev = &loop->ev_list[i];
      switch (ev->filter) {
        case EVFILT_SIGNAL:
          if (ev->ident == SIGINT) {
            close(loop->socket);
            close(kq);
            return;
          }
          break;
        default:
          {
            if (loop->ev_list[i].flags & EV_EOF) {
              int fd = loop->ev_list[i].ident;
              struct kevent ev_set;
              EV_SET(&ev_set, fd, EVFILT_READ, EV_DELETE, 0, 0, nullptr);
              kevent(kq, &ev_set, 1, nullptr, 0, nullptr);
              close(fd);
            } else if (loop->ev_list[i].ident == loop->socket) {
              struct sockaddr_storage addr;
              socklen_t socklen = sizeof(addr);
              int fd = accept(loop->ev_list[i].ident, (struct sockaddr*)&addr, &socklen);
              if (fd == -1) {
                printf("Failed to accept\n");
              } else {
                struct kevent add_read_socket;
                EV_SET(&add_read_socket, fd, EVFILT_READ, EV_ADD, 0, 0, nullptr);
                kevent(kq, &add_read_socket, 1, nullptr, 0, nullptr);
              }
            } else if (loop->ev_list[i].flags & EVFILT_READ) {
              const int max_len = 512;
              char buf[max_len + 1];
              ssize_t bytes_read = read(loop->ev_list[i].ident, buf, max_len);
              if (bytes_read == -1) {
                perror("receive: ");
                close(loop->ev_list[i].ident);
              } else {
                buf[max_len] = '\0';
                cb(loop->ev_list[i].ident, buf, bytes_read, loop->user);
              }
            }
          }
      }
    }
  }
}

