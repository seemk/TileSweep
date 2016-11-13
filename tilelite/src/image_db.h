#pragma once

#include <stdint.h>
#include "image.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  struct sqlite3* db;
  struct sqlite3_stmt* fetch_query;
  struct sqlite3_stmt* insert_position;
  struct sqlite3_stmt* insert_image;
} image_db;

image_db* image_db_open(const char* db_file);
void image_db_close(image_db* db);
int32_t image_db_fetch(const image_db* db, uint64_t position_hash,
                       int32_t width, int32_t height, image* img);
int32_t image_db_add_position(image_db* db, uint64_t position_hash,
                              uint64_t image_hash);
int32_t image_db_add_image(image_db* db, const image* img, uint64_t image_hash);

#ifdef __cplusplus
}
#endif
