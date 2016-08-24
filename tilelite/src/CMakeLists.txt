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
  tilelite.cpp
  tcp.cpp
  tilelite_config.cpp
  tl_time.cpp

  tile_renderer.cpp
  tl_math.cpp

  image_db.cpp

  hash/xxhash.c

  ini/ini.c
  parg/parg.c
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
