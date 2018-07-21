find_package(Threads)

include_directories(
  json
  ${MAPNIK_INCLUDE_DIR}
  "${CMAKE_SOURCE_DIR}/third_party/h2o/include"
)

add_executable(tilesweep
  hash/xxhash.c
  image_db.c
  ini/ini.c
  json/parson.c
  query.c
  parg/parg.c
  platform.c
  poly_fill.c
  poly_hit_test.c
  prerender.c
  stats.c
  sqlite3/sqlite3.c
  taskpool.c
  tcp.c
  tile_renderer.cpp
  tilesweep.c
  ts_cache.c
  ts_math.c
  ts_options.c
  ts_queue.c
  ts_rng.c
  ts_sema.c
  ts_task.c
  ts_tile.c
  ts_write_queue.c
)

if (WITH_MAPNIK)
  set(ICU_LIB "icuuc")
else(WITH_MAPNIK)
  target_compile_definitions(tilesweep PRIVATE TC_NO_MAPNIK=1)
endif (WITH_MAPNIK)

if (BUILD_TESTS)
add_executable(tests
  poly_hit_test.c
  poly_fill.c
  ts_queue.c
  test/tests.c
  ts_math.c
  query.c
)

ADD_DEPENDENCIES(tests libh2o libh2o-evloop)

target_link_libraries(tests
  m
)
endif(BUILD_TESTS)

ADD_DEPENDENCIES(tilesweep libh2o libh2o-evloop)

target_link_libraries(
  tilesweep
  ${MAPNIK_LIBRARY}
  ${ICU_LIB}
  libh2o-evloop
  ${CMAKE_THREAD_LIBS_INIT}
  ${CMAKE_DL_LIBS}
  m
)

file(COPY "${CMAKE_SOURCE_DIR}/ui" DESTINATION "${ARTIFACT_DIR}")
add_custom_command(TARGET tilesweep POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/conf/conf.ini ${ARTIFACT_DIR}
)
