#pragma once

#include <sys/event.h>
#include "common.h"

struct ev_loop_kqueue {
  int socket;
  int kq;
  struct kevent ev_set;
  struct kevent ev_list[MAX_EVENTS];
  void* user = nullptr;
};

bool ev_loop_kqueue_init(ev_loop_kqueue* loop, int socket);
void ev_loop_kqueue_run(ev_loop_kqueue* loop, void (*cb)(int, const char*, int, void*));
