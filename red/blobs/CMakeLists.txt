cmake_minimum_required(VERSION 2.6)
project(blobs)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11 -W -Wall -Wextra")

#find_path(LIBMAGIC_INC NAMES magic.h)
#include_directories(${LIBMAGIC_INC})

find_library(MAGIC magic)
include_directories(${MAGIC_INCLUDE_DIRS})

add_executable(blobs blobs.c)
target_link_libraries(blobs ${MAGIC})
