#pragma once

#include <stdint.h>

enum tl_message_type {
  tlm_none = 0,
  tlm_byte = 1,
  tlm_i32 = 2
};

struct tl_message {
  int len;
  tl_message_type next;
  const char* begin;
  const char* tag_cursor;
  const char* data_cursor;
};

int tlm_start(const char* data, int len, tl_message* message);
tl_message_type tlm_next(tl_message* message);
uint8_t tlm_read_byte(tl_message* message);
int32_t tlm_read_i32(tl_message* message);
