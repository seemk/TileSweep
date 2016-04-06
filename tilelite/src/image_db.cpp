#include "image_db.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "sqlite3/sqlite3.h"

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
      "primary key not null, image_hash integer not null);",
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


  image_db* db = (image_db*)calloc(1, sizeof(image_db));
  db->db = sqlite_db;
  db->fetch_query = fetch_query;
  db->insert_position = insert_position;

  return db;
}

void image_db_close(image_db* db) {
  sqlite3_finalize(db->fetch_query);
  sqlite3_close_v2(db->db);
  free(db);
}

int image_db_fetch(const image_db* db, int64_t position_hash, image* img) {
  sqlite3_reset(db->fetch_query);

  sqlite3_bind_int64(db->fetch_query, 1, position_hash);

  if (sqlite3_step(db->fetch_query) == SQLITE_ROW) {
    const void* blob = sqlite3_column_blob(db->fetch_query, 0);
    int num_bytes = sqlite3_column_bytes(db->fetch_query, 0);

    img->data = (uint8_t*)calloc(num_bytes, 1);
    memcpy(img->data, blob, num_bytes);
    img->len = num_bytes;

    return 0;
  }

  return -1;
}
