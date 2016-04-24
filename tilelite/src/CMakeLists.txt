add_library(sqlite
  sqlite3/sqlite3.c
)

find_package(Threads)

include_directories("/usr/local/include/mapnik")

include(CheckFunctionExists)

check_function_exists(kqueue HAVE_KQUEUE)
check_function_exists(epoll HAVE_EPOLL)

if (HAVE_KQUEUE)
  set(LOOP_SRC ev_loop_kqueue.cpp)
  add_definitions(-DTILELITE_KQUEUE)
elseif (HAVE_EPOLL)
  set(LOOP_SRC ev_loop_epoll.cpp)
  add_definitions(-DTILELITE_EPOLL)
else()
  message(FATAL_ERROR "epoll or kqueue required")
endif()

add_definitions(-DLODEPNG_NO_COMPILE_CPP)

add_library(tl_message_static STATIC
  tl_message.cpp
)

add_library(tl_message SHARED
  tl_message.cpp
)

add_executable(tilelite
  tilelite_server.cpp
  ${LOOP_SRC}
  tcp.cpp
  tilelite.cpp
  tile.cpp
  tile_renderer.cpp
  tl_time.cpp
  image_db.cpp

  cmp.c

  hash/MurmurHash2.cpp

  ini/ini.c
)

add_executable(tl_message_test
  test/tl_message_test.cpp
)

target_link_libraries(tl_message_test
  tl_message_static
)

target_link_libraries(
  tilelite
  sqlite
  mapnik
  icuuc
  ${CMAKE_THREAD_LIBS_INIT}
  ${CMAKE_DL_LIBS}
)
