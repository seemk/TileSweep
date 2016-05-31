#include "tcp.h"
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdio.h>
#include <netdb.h>
#include <unistd.h>
#include "tl_log.h"

int bind_tcp(const char* host, const char* port) {
  struct addrinfo hints;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  struct addrinfo* results;
  int rv = getaddrinfo(host, port, &hints, &results);

  if (rv != 0) {
    tl_log("getaddrinfo: %s", gai_strerror(rv));
    return -1;
  }

  int fd = -1;

  struct addrinfo* result;
  for (result = results; result != nullptr; result = result->ai_next) {
    fd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (fd == -1) {
      continue;
    }

    int enable = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
      tl_log("setsockopt(SO_REUSEADDR) failed");
    }

    if (bind(fd, result->ai_addr, result->ai_addrlen) == 0) {
      break;
    }

    close(fd);
  }

  if (result == nullptr) {
    tl_log("failed to bind socket");
    return -1;
  }

  freeaddrinfo(result);

  return fd;
}
