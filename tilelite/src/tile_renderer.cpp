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

latlon xyz_latlon(double x, double y, double z) {
  latlon res;

  double n = std::pow(2.0, z);
  res.longitude = x / n * 360.0 - 180.0;
  res.latitude =
      std::atan(std::sinh(PI * (1.0 - 2.0 * y / n))) * 180.0 / PI;
  return res;
}

bool render_tile(tile_renderer* renderer, const tile* tile, image* image) {
  latlon p1 = xyz_latlon(double(tile->x), double(tile->y), double(tile->z));
  latlon p2 = xyz_latlon(double(tile->x + 1), double(tile->y + 1), double(tile->z));

  mapnik::lonlat2merc(&p1.longitude, &p1.latitude, 1);
  mapnik::lonlat2merc(&p2.longitude, &p2.latitude, 1);

  mapnik::box2d<double> bbox(p1.longitude, p1.latitude, p2.longitude,
                             p2.latitude);
  renderer->map->resize(tile->w, tile->h);
  renderer->map->zoom_to_box(bbox);
  if (renderer->map->buffer_size() == 0) {
    renderer->map->set_buffer_size(96);
  }

  mapnik::image_rgba8 buf(tile->w, tile->h);
  mapnik::agg_renderer<mapnik::image_rgba8> ren(*renderer->map, buf);

  try {
    ren.apply();
  } catch (std::exception& e) {
    fprintf(stderr, "error rendering tile:\n %s\n", e.what());
    return false;
  }

  image->width = tile->w;
  image->height = tile->h;
  image->data = stbi_write_png_to_mem(buf.bytes(), buf.row_size(), tile->w, tile->h, 4,
                                      &image->len);
  return true;
}

void tile_renderer_destroy(tile_renderer* renderer) {
  if (renderer->map) delete renderer->map;
}
