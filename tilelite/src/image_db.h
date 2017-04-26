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
  struct sqlite3_stmt* existing_position;
  int32_t inserts;
} image_db;

typedef struct {
  image img;
  uint64_t image_hash;
  uint64_t position_hash;
} image_db_insert;

image_db* image_db_open(const char* db_file);
int32_t image_db_init(image_db* db, const char* db_file);
void image_db_close(image_db* db);
int32_t image_db_fetch(const image_db* db, uint64_t position_hash,
                       int32_t width, int32_t height, image* img);
int32_t image_db_exists(image_db* db, uint64_t position_hash);
int32_t image_db_insert_batch(image_db* db, const image_db_insert* inserts, int32_t count);

#ifdef __cplusplus
}
#endif
