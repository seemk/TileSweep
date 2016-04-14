#include "ev_loop_kqueue.h"
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>

bool ev_loop_kqueue_init(ev_loop_kqueue* loop, int socket, void* user) {
  loop->socket = socket;
  loop->kq = kqueue();
  loop->user = user;
  EV_SET(&loop->ev_set, socket, EVFILT_READ, EV_ADD, 0, 0, NULL);
  if (kevent(loop->kq, &loop->ev_set, 1, NULL, 0, NULL) == -1) {
    return false;
  }

  return true;
}

void ev_loop_kqueue_run(ev_loop_kqueue* loop, void (*cb)(int, const char*, int, void*)) {
  int kq = loop->kq;
  for (;;) {
    int n = kevent(kq, NULL, 0, loop->ev_list, 128, NULL);
    if (n < 1) {
      printf("kevent error\n");
    }

    for (int i = 0; i < n; i++) {
      if (loop->ev_list[i].flags & EV_EOF) {
        int fd = loop->ev_list[i].ident;
        EV_SET(&loop->ev_set, fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
        kevent(kq, &loop->ev_set, 1, NULL, 0, NULL);
        close(fd);
      } else if (loop->ev_list[i].ident == loop->socket) {
        struct sockaddr_storage addr;
        socklen_t socklen = sizeof(addr);
        int fd = accept(loop->ev_list[i].ident, (struct sockaddr*)&addr, &socklen);
        if (fd == -1) {
          printf("Failed to accept\n");
        } else {
          EV_SET(&loop->ev_set, fd, EVFILT_READ, EV_ADD, 0, 0, NULL);
          kevent(kq, &loop->ev_set, 1, NULL, 0, NULL);
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
        }
  
        cb(loop->ev_list[i].ident, buf, max_len, loop->user);
      }
    }
  }
}

