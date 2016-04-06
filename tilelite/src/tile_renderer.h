#pragma once

namespace mapnik {
class Map;
};

struct tile_renderer {
  mapnik::Map* map;
};

void register_plugins(const char* plugins_path);
void register_fonts(const char* fonts_path);

int tile_renderer_init(tile_renderer* renderer, const char* mapnik_xml_path);
