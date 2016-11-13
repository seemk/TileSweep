#pragma once

typedef struct {
  const char* plugins;
  const char* fonts;
  const char* database;
  const char* host;
  const char* port;
  const char* rendering;
  const char* mapnik_xml;
} tl_options;

tl_options tl_options_parse(int argc, char** argv);
