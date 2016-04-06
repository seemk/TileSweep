#include "tile_renderer.h"
#include <mapnik/map.hpp>
#include <mapnik/load_map.hpp>
#include <mapnik/datasource.hpp>
#include <mapnik/datasource_cache.hpp>

void register_plugins(const char* plugins_path) {
  mapnik::datasource_cache::instance().register_datasources(plugins_path);
}

void register_fonts(const char* fonts_path) {
  mapnik::freetype_engine::register_fonts(fonts_path, true);
}

int tile_renderer_init(tile_renderer* renderer, const char* mapnik_xml_path) {
  renderer->map = new mapnik::Map();
  mapnik::load_map(*renderer->map, mapnik_xml_path);
}
