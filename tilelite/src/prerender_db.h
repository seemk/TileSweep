#pragma once

#include "tl_math.h"
#include "prerender.h"

typedef struct {
  struct sqlite3* conn;
  struct sqlite3_stmt* new_job_stmt;
} prerender_db;

prerender_db* prerender_db_open(const char* file);
int64_t prerender_db_add_job(prerender_db* db, const prerender_req* req);
void prerender_db_close(prerender_db* db);
