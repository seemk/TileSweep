#include "image_db.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sqlite3/sqlite3.h"
#include "tl_log.h"

image_db* image_db_open(const char* db_file) {
  image_db* db = (image_db*)calloc(1, sizeof(image_db));
  if (!image_db_init(db, db_file)) {
    free(db);
    return NULL;
  }

  return db;
}

int32_t image_db_init(image_db* db, const char* db_file) {
  sqlite3* sqlite_db = NULL;
  int res = sqlite3_open(db_file, &sqlite_db);

  if (res != SQLITE_OK) {
    return 0;
  }

  char* err_msg = NULL;
  sqlite3_exec(sqlite_db, "PRAGMA journal_mode=WAL", NULL, NULL, &err_msg);
  sqlite3_wal_autocheckpoint(sqlite_db, 0);

  res = sqlite3_exec(
      sqlite_db,
      "CREATE TABLE IF NOT EXISTS image ("
      "image_hash integer primary key not null,"
      "data blob not null,"
      "width integer not null,"
      "height integer not null);"
      "CREATE TABLE IF NOT EXISTS tile ("
      "location_hash integer not null,"
      "image_hash integer not null);"
      "CREATE INDEX IF NOT EXISTS tile_pos_hash_index ON tile (location_hash);"
      "CREATE INDEX IF NOT EXISTS tile_image_hash_index ON tile (image_hash);",
      NULL, NULL, &err_msg);

  if (res != SQLITE_OK) {
    tl_log("%s", err_msg);
    sqlite3_close_v2(sqlite_db);
    return 0;
  }

  sqlite3_stmt* fetch_query = NULL;
  res = sqlite3_prepare_v2(sqlite_db,
                           "SELECT image.data, image.width, image.height "
                           "FROM tile JOIN image ON tile.image_hash "
                           "= image.image_hash WHERE tile.location_hash = ? "
                           "AND image.width = ? AND image.height = ?",
                           -1, &fetch_query, NULL);

  if (res != SQLITE_OK) {
    tl_log("%s", sqlite3_errmsg(sqlite_db));
    sqlite3_close_v2(sqlite_db);
    return 0;
  }

  sqlite3_stmt* insert_position = NULL;
  res = sqlite3_prepare_v2(sqlite_db, "INSERT INTO tile VALUES (?, ?)", -1,
                           &insert_position, NULL);

  if (res != SQLITE_OK) {
    tl_log("%s", sqlite3_errmsg(sqlite_db));
    sqlite3_close_v2(sqlite_db);
    return 0;
  }

  sqlite3_stmt* insert_image = NULL;
  sqlite3_prepare_v2(sqlite_db,
                     "INSERT OR IGNORE INTO image VALUES (?, ?, ?, ?)", -1,
                     &insert_image, NULL);

  sqlite3_stmt* existing_pos = NULL;
  sqlite3_prepare_v2(sqlite_db,
                     "SELECT count(*) FROM tile WHERE location_hash = ?", -1,
                     &existing_pos, NULL);

  db->db = sqlite_db;
  db->fetch_query = fetch_query;
  db->insert_position = insert_position;
  db->insert_image = insert_image;
  db->existing_position = existing_pos;

  sqlite3_busy_timeout(sqlite_db, 2000);
  return 1;
}

void image_db_close(image_db* db) {
  sqlite3_finalize(db->fetch_query);
  sqlite3_finalize(db->insert_position);
  sqlite3_finalize(db->insert_image);
  sqlite3_close_v2(db->db);
  free(db);
}

int32_t image_db_fetch(const image_db* db, uint64_t position_hash, int width,
                       int height, image* img) {
  sqlite3_reset(db->fetch_query);

  sqlite3_bind_int64(db->fetch_query, 1, position_hash);
  sqlite3_bind_int(db->fetch_query, 2, width);
  sqlite3_bind_int(db->fetch_query, 3, height);

  int res = sqlite3_step(db->fetch_query);

  if (res == SQLITE_BUSY) {
    tl_log("image fetch failed: BUSY");
  }

  if (res == SQLITE_ROW) {
    const void* blob = sqlite3_column_blob(db->fetch_query, 0);
    int num_bytes = sqlite3_column_bytes(db->fetch_query, 0);
    img->width = sqlite3_column_int(db->fetch_query, 1);
    img->height = sqlite3_column_int(db->fetch_query, 2);

    img->len = num_bytes;
    img->data = (uint8_t*)calloc(num_bytes, 1);
    memcpy(img->data, blob, num_bytes);

    return 1;
  }

  return 0;
}

int32_t image_db_add_position(image_db* db, uint64_t position_hash,
                              uint64_t image_hash) {
  sqlite3_stmt* query = db->insert_position;

  sqlite3_reset(query);

  sqlite3_bind_int64(query, 1, position_hash);
  sqlite3_bind_int64(query, 2, image_hash);

  int res = sqlite3_step(query);

  if (res != SQLITE_DONE) {
    tl_log("add position failed %d: %s", res, sqlite3_errstr(res));
  }

  return res == SQLITE_DONE ? 1 : 0;
}

int32_t image_db_exists(image_db* db, uint64_t position_hash) {
  sqlite3_stmt* query = db->existing_position;

  sqlite3_reset(query);
  sqlite3_bind_int64(query, 1, position_hash);

  int res = sqlite3_step(query);

  int32_t count = 0;
  while (sqlite3_step(query) == SQLITE_ROW) {
    count += sqlite3_column_int(query, 0);
  }

  return count;
}

int32_t image_db_add_image(image_db* db, const image* img,
                           uint64_t image_hash) {
  sqlite3_stmt* query = db->insert_image;

  sqlite3_reset(query);

  sqlite3_bind_int64(query, 1, image_hash);
  sqlite3_bind_blob(query, 2, img->data, img->len, SQLITE_STATIC);
  sqlite3_bind_int(query, 3, img->width);
  sqlite3_bind_int(query, 4, img->height);

  int res = sqlite3_step(query);

  if (res == SQLITE_DONE) {
    db->inserts++;

    if (db->inserts >= 1024) {
      tl_log("image db WAL checkpoint begin");
      int wal_res = sqlite3_wal_checkpoint_v2(
          db->db, NULL, SQLITE_CHECKPOINT_RESTART, NULL, NULL);
      if (wal_res == SQLITE_OK) {
        db->inserts = 0;
        tl_log("image db WAL checkpoint done");
      } else {
        tl_log("image db WAL checkpoint failed: %s", sqlite3_errmsg(db->db));
      }
    }
  } else {
    tl_log("add image failed %d: %s", res, sqlite3_errstr(res));
  }

  return res == SQLITE_DONE ? 1 : 0;
}
