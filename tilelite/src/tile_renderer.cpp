#include "tile_renderer.h"
#include <mapnik/map.hpp>
#include <mapnik/load_map.hpp>
#include <mapnik/datasource.hpp>
#include <mapnik/datasource_cache.hpp>
#include <mapnik/agg_renderer.hpp>
#include <mapnik/image_util.hpp>
#include <mapnik/image.hpp>
#include <mapnik/well_known_srs.hpp>
#include "image.h"
#include "tile.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

const double PI = 3.14159265358979323846;

struct latlon {
  double latitude;
  double longitude;
};

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

latlon xyz_latlon(int x, int y, int z) {
  latlon res;

  double n = std::pow(2.0, double(z));
  res.longitude = double(x) / n * 360.0 - 180.0;
  res.latitude =
      std::atan(std::sinh(PI * (1.0 - 2.0 * double(y) / n))) * 180.0 / PI;
  return res;
}

mapnik::box2d<double> tile_bbox(const tile* tile) {}

bool render_tile(tile_renderer* renderer, const tile* tile, image* image) {
   latlon p1 = xyz_latlon(tile->x, tile->y, tile->z);
   latlon p2 = xyz_latlon(tile->x + 1, tile->y + 1, tile->z);

  mapnik::lonlat2merc(&p1.longitude, &p1.latitude, 1);
  mapnik::lonlat2merc(&p2.longitude, &p2.latitude, 1);

  mapnik::box2d<double> bbox(p1.longitude, p1.latitude, p2.longitude,
                             p2.latitude);
  renderer->map->resize(tile->w, tile->h);
  renderer->map->zoom_to_box(bbox);

  mapnik::image_rgba8 buf(tile->w, tile->h);
  mapnik::agg_renderer<mapnik::image_rgba8> ren(*renderer->map, buf);
  ren.apply();

  image->data = stbi_write_png_to_mem(buf.bytes(), buf.row_size(), tile->w, tile->h, 4,
                                      &image->len);
  return true;
}
