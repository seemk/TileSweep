#pragma once

enum tl_message_type {
  tlm_end = 0,
  tlm_integer = 1
};

struct tl_message {
  int message_len;
  const char* message;
  const char* head;
};

int tlm_start(const char* data, int len, tl_message* message);
int tlm_next(tl_message* message);
