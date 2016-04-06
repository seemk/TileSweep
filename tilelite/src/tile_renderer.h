#pragma once

namespace mapnik {
class Map;
};

struct tile_renderer {
  mapnik::Map* map;
};

bool register_plugins(const char* plugins_path);
bool register_fonts(const char* fonts_path);
bool tile_renderer_init(tile_renderer* renderer, const char* mapnik_xml_path);
