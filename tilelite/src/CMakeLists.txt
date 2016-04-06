add_library(sqlite
  sqlite3/sqlite3.c
)

find_package(Threads)

include_directories("/usr/local/include/mapnik")

add_executable(tilelite
  tilelite.cpp
  tile_renderer.cpp
  image_db.cpp

  ini/ini.c
)

target_link_libraries(
  tilelite
  sqlite
  mapnik
  icuuc
  ${CMAKE_THREAD_LIBS_INIT}
  ${CMAKE_DL_LIBS}
)
