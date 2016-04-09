#include "tile.h"
#include <string.h>
#include <stdlib.h>

tile parse_tile(const char* s, int len) {
  int vals[5] = {0};
  char buf[16];
  int idx = 0;
  int v_idx = 0;

  for (int i = 0; i < len; i++) {
    char c = s[i];
    if (c == ',') {
      buf[idx] = '\0';
      vals[v_idx++] = atoi(buf);
      idx = 0;

      if (v_idx == 4) {
        // parse remaining after last comma
        for (int j = i + 1; j < len; j++) {
          buf[idx++] = s[j];
        }
        buf[idx] = '\0';
        vals[v_idx] = atoi(buf);
        break;
      }
    } else {
      buf[idx++] = c;
    }
  }

  tile c;
  memcpy(&c, vals, sizeof(vals));
  return c;
}
