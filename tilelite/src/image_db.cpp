#include "image_db.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "sqlite3/sqlite3.h"
#include "image.h"

image_db* image_db_open(const char* db_file) {
  sqlite3* sqlite_db = NULL;
  int res = sqlite3_open(db_file, &sqlite_db);

  if (res != SQLITE_OK) {
    return NULL;
  }

  char* err_msg = NULL;

  res =
      sqlite3_exec(sqlite_db, "PRAGMA journal_mode=WAL", NULL, NULL, &err_msg);

  if (res != SQLITE_OK) {
    fprintf(stderr, "%s\n", err_msg);
    return NULL;
  }

  res = sqlite3_exec(
      sqlite_db,
      "CREATE TABLE IF NOT EXISTS image (image_hash integer primary "
      "key not null, data blob not null);"
      "CREATE TABLE IF NOT EXISTS tile (location_hash integer "
      "primary key not null, image_hash integer not null);"
      "CREATE INDEX IF NOT EXISTS tile_image_hash_index ON tile (image_hash);",
      NULL, NULL, &err_msg);

  if (res != SQLITE_OK) {
    fprintf(stderr, "%s\n", err_msg);
    sqlite3_close_v2(sqlite_db);
    return NULL;
  }

  sqlite3_stmt* fetch_query = NULL;
  res =
      sqlite3_prepare_v2(sqlite_db,
                         "SELECT data FROM tile JOIN image ON tile.image_hash "
                         "= image.image_hash WHERE tile.location_hash = ?",
                         -1, &fetch_query, NULL);

  if (res != SQLITE_OK) {
    fprintf(stderr, "%s\n", sqlite3_errmsg(sqlite_db));
    sqlite3_close_v2(sqlite_db);
    return NULL;
  }

  sqlite3_stmt* insert_position = NULL;
  res = sqlite3_prepare_v2(sqlite_db,
      "INSERT INTO tile VALUES (?, ?)", -1, &insert_position, NULL);

  if (res != SQLITE_OK) {
    fprintf(stderr, "%s\n", sqlite3_errmsg(sqlite_db));
    sqlite3_close_v2(sqlite_db);
    return NULL;
  }

  sqlite3_stmt* insert_image = NULL;
  res = sqlite3_prepare_v2(sqlite_db,
    "INSERT INTO image VALUES (?, ?)", -1, &insert_image, NULL);


  image_db* db = (image_db*)calloc(1, sizeof(image_db));
  db->db = sqlite_db;
  db->fetch_query = fetch_query;
  db->insert_position = insert_position;
  db->insert_image = insert_image;

  return db;
}

void image_db_close(image_db* db) {
  sqlite3_finalize(db->fetch_query);
  sqlite3_finalize(db->insert_position);
  sqlite3_finalize(db->insert_image);
  sqlite3_close_v2(db->db);
  free(db);
}

bool image_db_fetch(const image_db* db, uint64_t position_hash, image* img) {
  sqlite3_reset(db->fetch_query);

  sqlite3_bind_int64(db->fetch_query, 1, position_hash);

  int res = sqlite3_step(db->fetch_query);
  if (res == SQLITE_ROW) {
    const void* blob = sqlite3_column_blob(db->fetch_query, 0);
    int num_bytes = sqlite3_column_bytes(db->fetch_query, 0);

    img->data = (uint8_t*)calloc(num_bytes, 1);
    memcpy(img->data, blob, num_bytes);
    img->len = num_bytes;

    return true;
  }

  return false;
}

bool image_db_add_position(image_db* db, uint64_t position_hash, uint64_t image_hash) {
  sqlite3_stmt* query = db->insert_position;

  sqlite3_reset(query);

  sqlite3_bind_int64(query, 1, position_hash);
  sqlite3_bind_int64(query, 2, image_hash);

  return sqlite3_step(query) == SQLITE_DONE;
}

bool image_db_add_image(image_db* db, const image* img, uint64_t image_hash) {
  sqlite3_stmt* query = db->insert_image;

  sqlite3_reset(query);

  sqlite3_bind_int64(query, 1, image_hash);
  sqlite3_bind_blob(query, 2, img->data, img->len, SQLITE_STATIC);

  return sqlite3_step(query) == SQLITE_DONE;
}
