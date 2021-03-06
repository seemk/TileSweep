#include "tile_renderer.h"
#include "platform.h"
#include "ts_math.h"

#if TC_NO_MAPNIK

struct tile_renderer {};

tile_renderer* tile_renderer_create(const char*, const char*, const char*) {
  return nullptr;
}

int32_t render_tile(tile_renderer*, const ts_tile*, image*) { return 0; }

void tile_renderer_destroy(tile_renderer*) {}

#else

#include <mapnik/agg_renderer.hpp>
#include <mapnik/datasource_cache.hpp>
#include <mapnik/image.hpp>
#include <mapnik/image_util.hpp>
#include <mapnik/load_map.hpp>
#include <mapnik/map.hpp>
#include <mapnik/well_known_srs.hpp>
#include <memory>
#include <mutex>

struct tile_renderer {
  tile_renderer(std::unique_ptr<mapnik::Map> map) : map(std::move(map)) {}
  std::unique_ptr<mapnik::Map> map;
};

void mapnik_global_init(const char* plugins_path, const char* fonts_path) {
  static std::once_flag done;

  std::call_once(done, [=]() {
    mapnik::logger::instance().set_severity(mapnik::logger::none);
    mapnik::datasource_cache::instance().register_datasources(plugins_path);
    mapnik::freetype_engine::register_fonts(fonts_path, true);
  });
}

tile_renderer* tile_renderer_create(const char* mapnik_xml_path,
                                    const char* plugins_path,
                                    const char* fonts_path) {
  try {
    mapnik_global_init(plugins_path, fonts_path);
    std::unique_ptr<mapnik::Map> map =
        std::unique_ptr<mapnik::Map>(new mapnik::Map());
    mapnik::load_map(*map, mapnik_xml_path);
    return new tile_renderer(std::move(map));
  } catch (std::exception& e) {
    ts_log("mapnik load error: %s", e.what());
  }

  return nullptr;
}

void tile_renderer_destroy(tile_renderer* renderer) { delete renderer; }

int32_t render_tile(tile_renderer* renderer, const ts_tile* tile,
                    image* image) {
  bounding_boxd merc_bb = tile_to_mercator(tile->x, tile->y, tile->z, tile->w);

  mapnik::box2d<double> bbox(merc_bb.top_left.x, merc_bb.top_left.y,
                             merc_bb.bot_right.x, merc_bb.bot_right.y);

  renderer->map->resize(tile->w, tile->h);
  renderer->map->zoom_to_box(bbox);
  if (renderer->map->buffer_size() == 0) {
    renderer->map->set_buffer_size(96);
  }

  assert(tile->w > 0 && tile->h > 0);
  mapnik::image_rgba8 buf(tile->w, tile->h);
  mapnik::agg_renderer<mapnik::image_rgba8> ren(*renderer->map, buf);

  try {
    ren.apply();
  } catch (std::exception& e) {
    ts_log("error rendering tile:\n %s", e.what());
    return 0;
  }

  std::string output_png = mapnik::save_to_string(buf, "png8");

  image->width = tile->w;
  image->height = tile->h;

  image->len = int(output_png.size());
  image->data = (uint8_t*)calloc(1, output_png.size());
  memcpy(image->data, output_png.data(), output_png.size());

  return 1;
}

#endif
