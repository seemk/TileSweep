find_package(Threads)

include_directories(
  ${MAPNIK_INCLUDE_DIR}
  "${CMAKE_SOURCE_DIR}/third_party/h2o/include"
)

add_executable(tilelite
  hash/xxhash.c
  image_db.cpp
  ini/ini.c
  parg/parg.c
  sqlite3/sqlite3.c
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
  ${MAPNIK_LIBRARY}
  icuuc
  libh2o-evloop
  ${CMAKE_THREAD_LIBS_INIT}
  ${CMAKE_DL_LIBS}
)

file(COPY "${CMAKE_SOURCE_DIR}/../ui" DESTINATION "${CMAKE_BINARY_DIR}")
