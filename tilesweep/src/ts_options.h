#pragma once

#include <stdint.h>

typedef struct {
  const char* plugins;
  const char* fonts;
  const char* database;
  const char* host;
  const char* port;
  const char* rendering;
  const char* mapnik_xml;
  const char* cache_size;
  const char* cache_log_factor_str;
  const char* cache_decay_seconds_str;
  uint64_t cache_size_bytes;
  double cache_log_factor;
  uint32_t cache_decay_seconds;
  int32_t rendering_enabled;
} ts_options;

ts_options ts_options_parse(int argc, char** argv);
