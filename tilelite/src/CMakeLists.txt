add_library(sqlite
  sqlite3/sqlite3.c
)

find_package(Threads)

include_directories("/usr/local/include/mapnik")

add_executable(tests
  test/tests.cpp

  hit_test.cpp
)

add_executable(tilelite
  hash/xxhash.c
  image_db.cpp
  ini/ini.c
  parg/parg.c
  tcp.cpp
  tile_renderer.cpp
  tilelite.cpp
  tl_math.cpp
  tl_options.cpp
  tl_tile.cpp
  tl_time.cpp
  tl_write_queue.cpp
)

target_link_libraries(
  tilelite
  sqlite
  mapnik
  icuuc
  libh2o-evloop
  ${CMAKE_THREAD_LIBS_INIT}
  ${CMAKE_DL_LIBS}
)
