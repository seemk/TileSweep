#pragma once

#include <memory>

struct tl_tile;
struct image;

namespace mapnik {
class Map;
};

struct tile_renderer {
  std::unique_ptr<mapnik::Map> map;
};

bool register_plugins(const char* plugins_path);
bool register_fonts(const char* fonts_path);
bool tile_renderer_init(tile_renderer* renderer, const char* mapnik_xml_path,
                        const char* plugins_path, const char* fonts_path);
bool render_tile(tile_renderer* renderer, const tl_tile* tile, image* image);
