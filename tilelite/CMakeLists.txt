project(TileLite)
cmake_minimum_required(VERSION 3.1)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/modules")

find_package(Mapnik REQUIRED)

SET(DISABLE_LIBUV ON)
add_definitions(-DH2O_USE_LIBUV=0)
add_subdirectory("third_party/h2o")

add_subdirectory(src)
