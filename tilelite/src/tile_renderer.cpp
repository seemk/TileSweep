#include "tile_renderer.h"
#include <mapnik/map.hpp>
#include <mapnik/load_map.hpp>
#include <mapnik/datasource.hpp>
#include <mapnik/datasource_cache.hpp>

bool register_plugins(const char* plugins_path) {
  mapnik::datasource_cache::instance().register_datasources(plugins_path);
  return 0;
}

bool register_fonts(const char* fonts_path) {
  return mapnik::freetype_engine::register_fonts(fonts_path, true);
}

bool tile_renderer_init(tile_renderer* renderer, const char* mapnik_xml_path) {
  renderer->map = new mapnik::Map();

  try {
    mapnik::load_map(*renderer->map, mapnik_xml_path);
  } catch (std::exception& e) {
    fprintf(stderr, "mapnik load error: %s\n", e.what());
    delete renderer->map;
    renderer->map = NULL;
    return false;
  }

  return true;
}
