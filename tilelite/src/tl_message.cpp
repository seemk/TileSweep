#include "tl_message.h"
#include <string.h>

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

int tlm_start(const char* data, int len, tl_message* message) {
  if (len < 3) {
    return 0;
  }

  message->len = len;
  message->next = tlm_none;
  message->begin = data;
  message->tag_cursor = data;

  while (data != '\0') {
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

  tl_message_type t = tlm_type(*message->tag_cursor++);
  return t;
}
