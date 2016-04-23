#include "tl_message.h"
#include <string.h>
#include <stdio.h>

static tl_message_type tlm_type(char c) {
  switch (c) {
    case 'b':
      return tlm_byte;
    case 'i':
      return tlm_i32;
    default:
      return tlm_none;
  }
}

int tlm_start(const uint8_t* data, int len, tl_message* message) {
  if (len < 3) {
    return 0;
  }

  message->len = len;
  message->next = tlm_none;
  message->begin = data;
  message->tag_cursor = data;

  while (*data != '\0') {
    data++;
  }

  if (data - message->begin < len - 1) {
    message->data_cursor = data + 1;
    return 1;
  }

  return 0;
}

tl_message_type tlm_next(tl_message* message) {
  if (message->tag_cursor == '\0') {
    return tlm_none;
  }

  return tlm_type(*message->tag_cursor++);
}

uint8_t tlm_read_byte(tl_message* message) {
  uint8_t result = *message->data_cursor;

  message->data_cursor++;

  return result;
}

int32_t tlm_read_i32(tl_message* message) {
  const uint8_t* p = message->data_cursor;
  int32_t res = int32_t(p[0]) << 24 | int32_t(p[1]) << 16 | int32_t(p[2]) << 8 |
                int32_t(p[3]);

  message->data_cursor += 4;

  return res;
}
