#pragma once

struct tile {
  int z;
  int x;
  int y;
  int w;
  int h;
};

tile parse_tile(const char* s, int len);
