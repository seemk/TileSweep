#include "ts_tile.h"
#include <ctype.h>
#include <string.h>

static int32_t strntoi(const char* s, size_t len) {
  int32_t n = 0;

  for (size_t i = 0; i < len; i++) {
    n = n * 10 + s[i] - '0';
  }

  return n;
}

ts_tile parse_tile(const char* s, size_t len) {
  size_t begin = 0;
  size_t end = 0;
  int32_t part = 0;
  int32_t parsing_num = 0;
  int32_t params[5];
  memset(params, -1, sizeof(params));

  for (size_t i = 0; i < len; i++) {
    if (isdigit(s[i])) {
      if (!parsing_num) {
        begin = i;
        parsing_num = 1;
      }
      end = i;
    } else {
      if (parsing_num) {
        params[part++] = (int32_t)strntoi(&s[begin], end - begin + 1);
        parsing_num = 0;
      }
    }

    if (i == len - 1 && parsing_num) {
      params[part] = (int32_t)strntoi(&s[begin], end - begin + 1);
    }
  }

  ts_tile t = {params[0], params[1], params[2], params[3], params[4]};

  return t;
}

int32_t tile_valid(const ts_tile* t) {
  return t->w > 0 && t->h > 0 && t->x > -1 && t->y > -1 && t->z > -1;
}
