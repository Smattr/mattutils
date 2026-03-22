## @file
## @brief Build system for libute
##
## All content in this file is in the public domain. Use it any way you wish.

add_executable(test
  src/cleanup.c
  src/main.c
  src/test-asp-mt.c
  src/test-asp-self-store-aba.c
  src/test-asp-st.c
  src/test-int128-cas.c
  src/test-int128-cas-ro.c
  src/test-int128-cas-fail.c
  src/test-int128-cas-fail-mt.c
  src/test-int128-cas-mt.c
  src/test-int128-load.c
  src/test-int128-load-mt.c
  src/test-int128-store.c
  src/test-int128-store-mt.c
  src/test-int128-xchg.c
  src/test-int128-xchg-mt.c
  src/test-print-int128-max.c
  src/test-print-int128-min.c
  src/test-print-int128-small.c
  src/test-print-uint128-max.c
  src/test-print-uint128-small.c
  src/test-putb.c
  src/test-set-basic.c
  src/test-uint128-cas.c
  src/test-uint128-cas-fail.c
  src/test-uint128-cas-fail-mt.c
  src/test-uint128-cas-mt.c
  src/test-uint128-load.c
  src/test-uint128-load-mt.c
  src/test-uint128-store.c
  src/test-uint128-store-load-tear.c
  src/test-uint128-store-mt.c
  src/test-uint128-xchg.c
  src/test-uint128-xchg-mt.c
)

# enable `open_memstream`
target_compile_definitions(test PRIVATE _GNU_SOURCE)

target_link_libraries(test PRIVATE libute)

if(CMAKE_USE_PTHREADS_INIT)
  target_compile_definitions(test PRIVATE USE_PTHREADS=1)
else()
  target_compile_definitions(test PRIVATE USE_PTHREADS=0)
endif()
target_link_libraries(test PRIVATE ${CMAKE_THREAD_LIBS_INIT})

add_custom_target(check
  COMMAND ${CMAKE_CURRENT_BINARY_DIR}/test
)

add_dependencies(check test)
