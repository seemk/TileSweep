#pragma once

#include <stdint.h>

struct image {
  int len;
  uint8_t* data;
};
struct sqlite3;
struct sqlite3_stmt;

struct image_db {
  sqlite3* db;
  sqlite3_stmt* fetch_query;
  sqlite3_stmt* insert_position;
  sqlite3_stmt* insert_image;
};

image_db* image_db_open(const char* db_file);
void image_db_close(image_db* db);
int image_db_fetch(const image_db* db, int64_t position_hash, image* img);
void image_db_add_position(image_db* db, int64_t position_hash, int64_t image_hash);
void image_db_add_image(image_db* db, const image* img);
