## @file
## @brief Build system for libute
##
## All content in this file is in the public domain. Use it any way you wish.

add_library(libute
  src/asp.c
  src/dword_atomic_cas.c
  src/dword_atomic_cas_n.c
  src/dword_atomic_load.c
  src/dword_atomic_store.c
  src/dword_atomic_xchg.c
  src/hash.c
  src/int128_atomic_cas.c
  src/int128_atomic_cas_n.c
  src/int128_atomic_load.c
  src/int128_atomic_store.c
  src/int128_atomic_xchg.c
  src/int128_put.c
  src/set_contains_.c
  src/set_free_.c
  src/set_insert_.c
  src/set_remove_.c
  src/set_size_.c
  src/uint128_atomic_cas.c
  src/uint128_atomic_cas_n.c
  src/uint128_atomic_load.c
  src/uint128_atomic_store.c
  src/uint128_atomic_xchg.c
  src/uint128_put.c
  src/putb.c
)

if(APPLE)
  target_compile_options(libute PRIVATE -fno-common)
endif()

CHECK_C_COMPILER_FLAG(-mcx16 HAS_MCX16)
if(HAS_MCX16)
  target_compile_options(libute PUBLIC -mcx16)
endif()

target_include_directories(libute
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
  PRIVATE
    src
)

set_target_properties(libute PROPERTIES PREFIX "")

install(TARGETS libute EXPORT libuteConfig
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(
  EXPORT libuteConfig
  FILE libuteConfig.cmake
  NAMESPACE ute::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ute
)
install(
  FILES uteConfig.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ute
)
