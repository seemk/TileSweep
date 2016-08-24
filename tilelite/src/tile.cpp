#include "tile.h"
#include <ctype.h>

static int strntoi(const char* s, size_t len) {
  int n = 0;

  for (size_t i = 0; i < len; i++) {
    n = n * 10 + s[i] - '0';
  }

  return n;
}

tile parse_tile(const char* s, size_t len) {
  size_t begin = 0;
  size_t end = 0;
  int part = 0;
  int parsing_num = 0;
  int params[5] = {-1};

  for (size_t i = 0; i < len; i++) {
    if (isdigit(s[i])) {
      if (!parsing_num) {
        begin = i;
        parsing_num = 1;
      }
      end = i;
    } else {
      if (parsing_num) {
        params[part++] = int(strntoi(&s[begin], end - begin + 1));
        parsing_num = 0;
      }
    }

    if (i == len - 1 && parsing_num) {
      params[part] = int(strntoi(&s[begin], end - begin + 1));
    }
  }

  tile t;
  t.w = params[0];
  t.h = params[1];
  t.x = params[2];
  t.y = params[3];
  t.z = params[4];

  return t;
}
