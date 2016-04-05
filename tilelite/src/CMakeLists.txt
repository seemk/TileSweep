add_library(sqlite
  sqlite3/sqlite3.c
)

find_package(Threads)

add_executable(tilelite
  tilelite.cpp
  image_db.cpp
)

target_link_libraries(
  tilelite
  sqlite
  ${CMAKE_THREAD_LIBS_INIT}
  ${CMAKE_DL_LIBS}
)
