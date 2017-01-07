#include "prerender_db.h"
#include <stdlib.h>
#include "prerender.h"
#include "sqlite3/sqlite3.h"
#include "tl_log.h"

prerender_db* prerender_db_open(const char* file) {
  sqlite3* sqlite_db = NULL;
  int res = sqlite3_open(file, &sqlite_db);

  if (res != SQLITE_OK) {
    return NULL;
  }

  char* err_msg = NULL;
  sqlite3_exec(sqlite_db, "PRAGMA journal_mode=WAL", NULL, NULL, &err_msg);

  res = sqlite3_exec(sqlite_db,
                     "CREATE TABLE IF NOT EXISTS prerender_job ("
                     "min_zoom integer not null,"
                     "max_zoom integer not null,"
                     "tile_size integer not null,"
                     "coordinates blob not null);",
                     NULL, NULL, &err_msg);

  if (res != SQLITE_OK) {
    tl_log("failed to create prerender table: %s", err_msg);
    sqlite3_close_v2(sqlite_db);
    return NULL;
  }

  sqlite3_stmt* new_job_stmt = NULL;
  res = sqlite3_prepare_v2(sqlite_db,
                           "INSERT INTO prerender_job VALUES (?, ?, ?, ?)", -1,
                           &new_job_stmt, NULL);

  if (res != SQLITE_OK) {
    tl_log("%s", sqlite3_errmsg(sqlite_db));
    sqlite3_close_v2(sqlite_db);
    return NULL;
  }

  prerender_db* db = (prerender_db*)calloc(1, sizeof(prerender_db));
  db->conn = sqlite_db;
  db->new_job_stmt = new_job_stmt;

  sqlite3_busy_timeout(sqlite_db, 1000);

  return db;
}

void prerender_db_close(prerender_db* db) {
  sqlite3_finalize(db->new_job_stmt);
  sqlite3_close_v2(db->conn);
  free(db);
}

int64_t prerender_db_add_job(prerender_db* db, const prerender_req* req) {
  sqlite3_reset(db->new_job_stmt);
  sqlite3_bind_int(db->new_job_stmt, 1, req->min_zoom);
  sqlite3_bind_int(db->new_job_stmt, 2, req->max_zoom);
  sqlite3_bind_int(db->new_job_stmt, 3, req->tile_size);
  sqlite3_bind_blob(db->new_job_stmt, 4, req->coordinates,
                    req->num_coordinates * sizeof(req->coordinates[0]),
                    SQLITE_STATIC);

  int res = sqlite3_step(db->new_job_stmt);

  if (res == SQLITE_DONE) {
    return sqlite3_last_insert_rowid(db->conn);
  }

  return -1;
}
