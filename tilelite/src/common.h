#pragma once

const int MAX_EVENTS = 128;
const int MAX_ZOOM_LEVEL = 18;
const int MAX_PRERENDER_COORDS = 64;

enum tl_status {
  tl_ok = 0,
  tl_no_tile = 1
};
