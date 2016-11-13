find_package(Threads)

include_directories(
  ${MAPNIK_INCLUDE_DIR}
  "${CMAKE_SOURCE_DIR}/third_party/h2o/include"
)

#add_executable(tilelite
#  hash/xxhash.c
#  image_db.c
#  ini/ini.c
#  parg/parg.c
#  sqlite3/sqlite3.c
#  tcp.c
#  tile_renderer.cpp
#  tilelite.cpp
#  tl_math.cpp
#  tl_options.cpp
#  tl_tile.c
#  tl_time.c
#  tl_write_queue.cpp
#)

add_executable(tilelite
  cpu.c
  tilelite.c
  hash/xxhash.c
  image_db.c
  tl_options.c
  tl_time.c
  tl_tile.c
  tl_math.c
  tcp.c
  tl_write_queue.cpp
  tile_renderer.cpp

  ini/ini.c
  sqlite3/sqlite3.c
  parg/parg.c
)

target_link_libraries(
  tilelite
  ${MAPNIK_LIBRARY}
  icuuc
  libh2o-evloop
  ${CMAKE_THREAD_LIBS_INIT}
  ${CMAKE_DL_LIBS}
  m
)

file(COPY "${CMAKE_SOURCE_DIR}/../ui" DESTINATION "${CMAKE_BINARY_DIR}")
