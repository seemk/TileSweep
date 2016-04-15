#pragma once

struct tile;
struct image;

namespace mapnik {
class Map;
};

struct tile_renderer {
  mapnik::Map* map = nullptr;
};

bool register_plugins(const char* plugins_path);
bool register_fonts(const char* fonts_path);
bool tile_renderer_init(tile_renderer* renderer, const char* mapnik_xml_path);
bool render_tile(tile_renderer* renderer, const tile* tile, image* image);
void tile_renderer_destroy(tile_renderer* renderer);
