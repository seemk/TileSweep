#pragma once

#include <stdint.h>

struct image;
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
bool image_db_fetch(const image_db* db, uint64_t position_hash, image* img);
bool image_db_add_position(image_db* db, uint64_t position_hash, uint64_t image_hash);
bool image_db_add_image(image_db* db, const image* img, uint64_t image_hash);