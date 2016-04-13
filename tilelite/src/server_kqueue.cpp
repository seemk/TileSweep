#include <sys/socket.h>
#include <sys/event.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include "tcp.h"
#include <stdio.h>

int main(int argc, char** argv) {

  struct addrinfo hints;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  int sfd = bind_tcp("9567");
  if (sfd == -1) {
    return 1;
  }

  int res = listen(sfd, SOMAXCONN);
  if (res == -1) {
    perror("listen");
    return 1;
  }

  printf("Local fd: %d\n", sfd);

  int kq = kqueue();

  struct kevent ev_set;
  EV_SET(&ev_set, sfd, EVFILT_READ, EV_ADD, 0, 0, NULL);
  if (kevent(kq, &ev_set, 1, NULL, 0, NULL) == -1) {
    printf("kevent failure\n");
    return 1;
  }

  struct kevent ev_list[128];
  for (;;) {
    printf("Waiting\n");
    int n = kevent(kq, NULL, 0, ev_list, 128, NULL);
    if (n < 1) {
      printf("kevent error\n");
    }

    printf("Events %d\n", n);

    for (int i = 0; i < n; i++) {
      if (ev_list[i].flags & EV_EOF) {
        printf("Disconnect\n");
        int fd = ev_list[i].ident;
        EV_SET(&ev_set, fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
        kevent(kq, &ev_set, 1, NULL, 0, NULL);
        close(fd);
      } else if (ev_list[i].ident == sfd) {
        struct sockaddr_storage addr;
        socklen_t socklen = sizeof(addr);
        int fd = accept(ev_list[i].ident, (struct sockaddr*)&addr, &socklen);
        printf("Accepting\n");
        if (fd == -1) {
          printf("Failed to accept\n");
        } else {
          EV_SET(&ev_set, fd, EVFILT_READ, EV_ADD, 0, 0, NULL);
          kevent(kq, &ev_set, 1, NULL, 0, NULL);
        }
      } else if (ev_list[i].flags & EVFILT_READ) {
        const int max_len = 512;
        char buf[max_len + 1];
        ssize_t bytes_read = read(ev_list[i].ident, buf, max_len);
        if (bytes_read == -1) {
          perror("receive: ");
          close(ev_list[i].ident);
        } else {
          buf[max_len] = '\0';
        }

        printf("%s\n", buf);
      }
    }
  }

  return 0;
}
