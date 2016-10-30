#include "tl_jobdb.h"
#include "sqlite3/sqlite3.h"

tl_jobdb::~tl_jobdb() {
  if (query_jobs) sqlite3_finalize(query_jobs);
  if (db) sqlite3_close_v2(db);
}

std::unique_ptr<tl_jobdb> tl_jobdb_open(const char* file_name) {
  auto db = std::unique_ptr<tl_jobdb>(new tl_jobdb());
  

  return db;
}

std::vector<tl_job> tl_jobdb_unfinished_jobs(tl_jobdb* db) {
  return std::vector<tl_job>();
}

