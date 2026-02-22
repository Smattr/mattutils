## @file
## @brief Build system for libute
##
## All content in this file is in the public domain. Use it any way you wish.

add_executable(test
  src/cleanup.c
  src/main.c
  src/test-print-int128-max.c
  src/test-print-int128-min.c
  src/test-print-uint128-max.c
)

# enable `open_memstream`
target_compile_definitions(test PRIVATE _GNU_SOURCE)

target_link_libraries(test PRIVATE libute)

add_custom_target(check
  COMMAND ${CMAKE_CURRENT_BINARY_DIR}/test
)

add_dependencies(check test)
