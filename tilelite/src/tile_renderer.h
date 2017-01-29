#pragma once
#include <stdint.h>
#include "image.h"
#include "tl_tile.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct tile_renderer tile_renderer;

tile_renderer* tile_renderer_create(const char* mapnik_xml_path,
                                    const char* plugins_path,
                                    const char* fonts_path);
void tile_renderer_destroy(tile_renderer* renderer);
int32_t render_tile(tile_renderer* renderer, const tl_tile* tile, image* image);

#ifdef __cplusplus
}
#endif
