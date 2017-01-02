find_package(Threads)

include_directories(
  json
  ${MAPNIK_INCLUDE_DIR}
  "${CMAKE_SOURCE_DIR}/third_party/h2o/include"
)

add_executable(tilelite
  cpu.c
  tilelite.c
  hash/xxhash.c
  image_db.c
  task.c
  task_queue.c
  taskpool.c
  tl_options.c
  tl_time.c
  tl_tile.c
  tl_math.c
  tcp.c
  tl_write_queue.cpp
  tile_renderer.cpp

  ini/ini.c
  json/parson.c
  sqlite3/sqlite3.c
  parg/parg.c
  rng/pcg_basic.h
)

add_executable(tests
  task.c
  task_queue.c
  test/tests.c
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
