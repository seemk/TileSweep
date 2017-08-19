#pragma once
#include <stdint.h>

typedef struct {
  int32_t width;
  int32_t height;
  int32_t len;
  uint8_t* data;
} image;
