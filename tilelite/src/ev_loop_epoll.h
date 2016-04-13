#pragma once

#include <sys/epoll.h>
#include "common.h"

struct ev_loop_epoll {
  int socket;
  int efd;
  void* user = nullptr;
  struct epoll_event event;
  struct epoll_event events[MAX_EVENTS];
  int sigfd;
};

bool ev_loop_epoll_init(ev_loop_epoll* loop, int socket);
void ev_loop_epoll_run(ev_loop_epoll* loop, void (*cb)(int, const char*, int, void*));
