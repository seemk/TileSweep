add_library(sqlite
  sqlite3/sqlite3.c
)

find_package(Threads)

include_directories("/usr/local/include/mapnik")

if (APPLE)
  set(SERVER_SRC server_kqueue.cpp)
else()
  set(SERVER_SRC tilelite_server.cpp)
endif()

add_executable(tilelite
  ${SERVER_SRC}
  tcp.cpp
  tilelite.cpp
  tile.cpp
  tile_renderer.cpp
  image_db.cpp

  hash/MurmurHash2.cpp

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
